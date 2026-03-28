#import "SurfaceViewController.h"

#include "jni.h"
#include <assert.h>
#include <dlfcn.h>

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "GL/osmesa.h"

#include "glfw_keycodes.h"
#include "ctxbridges/bridge_tbl.h"
#include "ctxbridges/osmesa_internal.h"
#include "utils.h"

int clientAPI;

__thread basic_render_window_t* currentBundle = NULL;

typedef jboolean (*MetalCraftIsSupportedProc)(void);
typedef void (*MetalCraftShutdownProc)(void);

static void selectBridgeForRenderer(NSString *renderer) {
    unsetenv("GALLIUM_DRIVER");

    if (renderer.length == 0 || [renderer isEqualToString:@"auto"] ||
        [renderer isEqualToString:@ RENDERER_NAME_GL4ES] ||
        [renderer isEqualToString:@ RENDERER_NAME_MOBILEGLUES] ||
        [renderer isEqualToString:@ RENDERER_NAME_KRYPTON_WRAPPER] ||
        [renderer isEqualToString:@ RENDERER_NAME_MTL_ANGLE]) {
        if (renderer.length == 0 || [renderer isEqualToString:@"auto"]) {
            renderer = @ RENDERER_NAME_GL4ES;
        }
        setenv("POJAV_RENDERER_BOOTSTRAP", renderer.UTF8String, 1);
        set_gl_bridge_tbl();
        return;
    }

    if ([renderer hasPrefix:@"libOSMesa"]) {
        setenv("GALLIUM_DRIVER", "zink", 1);
        setenv("POJAV_RENDERER_BOOTSTRAP", renderer.UTF8String, 1);
        set_osm_bridge_tbl();
        return;
    }

    NSLog(@"[Renderer] Unknown backend %@, falling back to %s", renderer, RENDERER_NAME_GL4ES);
    setenv("POJAV_RENDERER_BOOTSTRAP", RENDERER_NAME_GL4ES, 1);
    set_gl_bridge_tbl();
}

static void JNI_LWJGL_syncRendererProperties(const char* backend_c, const char* bootstrap_c) {
    if (runtimeJavaVMPtr == NULL) {
        return;
    }

    if ((backend_c == NULL || backend_c[0] == '\0') &&
        (bootstrap_c == NULL || bootstrap_c[0] == '\0')) {
        return;
    }

    if (backend_c == NULL || backend_c[0] == '\0') {
        backend_c = bootstrap_c;
    }
    if (bootstrap_c == NULL || bootstrap_c[0] == '\0') {
        bootstrap_c = backend_c;
    }

    JNIEnv *env;
    if ((*runtimeJavaVMPtr)->GetEnv(runtimeJavaVMPtr, (void **)&env, JNI_VERSION_1_4) != JNI_OK ||
        env == NULL) {
        return;
    }

    jstring libnameKey = (*env)->NewStringUTF(env, "org.lwjgl.opengl.libname");
    jstring backendKey = (*env)->NewStringUTF(env, "pojav.renderer.backend");
    jstring bootstrapKey = (*env)->NewStringUTF(env, "pojav.renderer.bootstrap");
    jstring backendValue = (*env)->NewStringUTF(env, backend_c);
    jstring bootstrapValue = (*env)->NewStringUTF(env, bootstrap_c);
    jclass clazz = (*env)->FindClass(env, "java/lang/System");
    if (libnameKey == NULL || backendKey == NULL || bootstrapKey == NULL ||
        backendValue == NULL || bootstrapValue == NULL || clazz == NULL) {
        if (libnameKey != NULL) (*env)->DeleteLocalRef(env, libnameKey);
        if (backendKey != NULL) (*env)->DeleteLocalRef(env, backendKey);
        if (bootstrapKey != NULL) (*env)->DeleteLocalRef(env, bootstrapKey);
        if (backendValue != NULL) (*env)->DeleteLocalRef(env, backendValue);
        if (bootstrapValue != NULL) (*env)->DeleteLocalRef(env, bootstrapValue);
        if (clazz != NULL) (*env)->DeleteLocalRef(env, clazz);
        return;
    }

    jmethodID method = (*env)->GetStaticMethodID(env, clazz, "setProperty", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
    if (method != NULL) {
        jobject oldValue = (*env)->CallStaticObjectMethod(env, clazz, method, libnameKey, bootstrapValue);
        if (oldValue != NULL) {
            (*env)->DeleteLocalRef(env, oldValue);
        }
        oldValue = (*env)->CallStaticObjectMethod(env, clazz, method, backendKey, backendValue);
        if (oldValue != NULL) {
            (*env)->DeleteLocalRef(env, oldValue);
        }
        oldValue = (*env)->CallStaticObjectMethod(env, clazz, method, bootstrapKey, bootstrapValue);
        if (oldValue != NULL) {
            (*env)->DeleteLocalRef(env, oldValue);
        }
    }
    (*env)->DeleteLocalRef(env, clazz);
    (*env)->DeleteLocalRef(env, bootstrapValue);
    (*env)->DeleteLocalRef(env, backendValue);
    (*env)->DeleteLocalRef(env, bootstrapKey);
    (*env)->DeleteLocalRef(env, backendKey);
    (*env)->DeleteLocalRef(env, libnameKey);
}

void pojavTerminate() {
    CallbackBridge_nativeSetInputReady(NO);
    if (getenv("POJAV_METALCRAFT_ENABLED") != NULL) {
        MetalCraftShutdownProc shutdownProc =
            (MetalCraftShutdownProc)dlsym(RTLD_DEFAULT, "MetalCraftJNI_Shutdown");
        if (shutdownProc != NULL) {
            shutdownProc();
        }
    }
    if (!br_terminate) return;
    br_terminate();
}

void* pojavGetCurrentContext() {
    return br_get_current();
}

int pojavInit(BOOL useStackQueue) {
    clientAPI = GLFW_OPENGL_API;
    isInputReady = 1;
    isUseStackQueueCall = useStackQueue;
    return JNI_TRUE;
}

int pojavInitOpenGL() {
    NSDictionary<NSString *, NSString *> *environment = NSProcessInfo.processInfo.environment;
    NSString *requestedRenderer = environment[@"AMETHYST_RENDERER"];
    NSString *backendRenderer = environment[@"POJAV_RENDERER_BACKEND"];
    if (backendRenderer.length == 0) {
        backendRenderer = requestedRenderer;
    }
    NSString *bootstrapRenderer = environment[@"POJAV_RENDERER_BOOTSTRAP"];
    if (bootstrapRenderer.length == 0) {
        bootstrapRenderer = backendRenderer;
    }
    if (bootstrapRenderer.length == 0) {
        bootstrapRenderer = @"auto";
    }
    BOOL useMetalCraft = [requestedRenderer isEqualToString:@ RENDERER_NAME_METALCRAFT];
    if (useMetalCraft) {
        if (backendRenderer.length == 0) {
            backendRenderer = requestedRenderer;
        }
        if (bootstrapRenderer.length == 0 ||
            [bootstrapRenderer isEqualToString:@"auto"] ||
            [bootstrapRenderer isEqualToString:requestedRenderer] ||
            [bootstrapRenderer isEqualToString:backendRenderer]) {
            bootstrapRenderer = @ RENDERER_NAME_METALCRAFT_BOOTSTRAP;
        }
    }
    selectBridgeForRenderer(bootstrapRenderer);
    bootstrapRenderer = @(getenv("POJAV_RENDERER_BOOTSTRAP"));
    if (backendRenderer.length == 0 || [backendRenderer isEqualToString:@"auto"]) {
        backendRenderer = bootstrapRenderer;
    }
    JNI_LWJGL_syncRendererProperties(backendRenderer.UTF8String, bootstrapRenderer.UTF8String);
    // Preload renderer library
    void *rendererHandle = dlopen([NSString stringWithFormat:@"@rpath/%@", bootstrapRenderer].UTF8String, RTLD_GLOBAL);
    if (rendererHandle == NULL) {
        NSLog(@"[Renderer] Failed to preload %@: %s", bootstrapRenderer, dlerror());
    }
    if (useMetalCraft) {
        void *metalCraftHandle = dlopen("@rpath/libMetalCraft.dylib", RTLD_GLOBAL);
        if (metalCraftHandle != NULL) {
            NSLog(@"[MetalCraft] Native library loaded via dlopen");
            MetalCraftIsSupportedProc isSupported =
                (MetalCraftIsSupportedProc)dlsym(metalCraftHandle, "MetalCraftJNI_IsSupported");
            if (isSupported != NULL && isSupported() == JNI_FALSE) {
                NSLog(@"[MetalCraft] MetalCraftJNI_IsSupported returned false, disabling");
                unsetenv("POJAV_METALCRAFT_ENABLED");
            } else {
                NSLog(@"[MetalCraft] MetalCraft is supported and enabled");
            }
        } else {
            NSLog(@"[MetalCraft] Failed to load libMetalCraft.dylib: %s", dlerror());
            unsetenv("POJAV_METALCRAFT_ENABLED");
        }
    }

    if (br_init == NULL) {
        return JNI_TRUE;
    }
    return !br_init();
    //return 0;
}

void pojavSetWindowHint(int hint, int value) {
    if (hint == GLFW_CLIENT_API) {
        clientAPI = value;
    } else {
        const char *requestedRenderer = getenv("AMETHYST_RENDERER");
        if (requestedRenderer == NULL || strcmp(requestedRenderer, "auto") != 0 ||
            hint != GLFW_CONTEXT_VERSION_MAJOR) {
            return;
        }

        switch (value) {
            case 1:
            case 2:
                setenv("AMETHYST_RENDERER", RENDERER_NAME_GL4ES, 1);
                setenv("POJAV_RENDERER_BACKEND", RENDERER_NAME_GL4ES, 1);
                setenv("POJAV_RENDERER_BOOTSTRAP", RENDERER_NAME_GL4ES, 1);
                unsetenv("GALLIUM_DRIVER");
                JNI_LWJGL_syncRendererProperties(RENDERER_NAME_GL4ES, RENDERER_NAME_GL4ES);
                break;
            // Avoid implicit MobileGlues switching; prefer Krypton for newer contexts.
            default:
                setenv("AMETHYST_RENDERER", RENDERER_NAME_KRYPTON_WRAPPER, 1);
                setenv("POJAV_RENDERER_BACKEND", RENDERER_NAME_KRYPTON_WRAPPER, 1);
                setenv("POJAV_RENDERER_BOOTSTRAP", RENDERER_NAME_KRYPTON_WRAPPER, 1);
                unsetenv("GALLIUM_DRIVER");
                JNI_LWJGL_syncRendererProperties(RENDERER_NAME_KRYPTON_WRAPPER, RENDERER_NAME_KRYPTON_WRAPPER);
                break;
        }
    }
}

void pojavSwapBuffers() {
    br_swap_buffers();
}

void pojavMakeCurrent(basic_render_window_t* window) {
    br_make_current(window);
}

void* pojavCreateContext(basic_render_window_t* contextSrc) {
    if (clientAPI == GLFW_NO_API) {
        // Game has selected Vulkan API to render
        return (__bridge void *)SurfaceViewController.surface.layer;
    }

    static BOOL inited = NO;
    if (!inited) {
        inited = YES;
        pojavInitOpenGL();
    }

    return br_init_context(contextSrc);
}

void pojavSwapInterval(int interval) {
    if (!br_swap_interval) return;
    br_swap_interval(interval);
}
