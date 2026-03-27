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

void JNI_LWJGL_changeRenderer(const char* value_c) {
    if (runtimeJavaVMPtr == NULL || value_c == NULL) {
        return;
    }

    JNIEnv *env;
    if ((*runtimeJavaVMPtr)->GetEnv(runtimeJavaVMPtr, (void **)&env, JNI_VERSION_1_4) != JNI_OK ||
        env == NULL) {
        return;
    }

    jstring key = (*env)->NewStringUTF(env, "org.lwjgl.opengl.libname");
    jstring value = (*env)->NewStringUTF(env, value_c);
    jclass clazz = (*env)->FindClass(env, "java/lang/System");
    if (key == NULL || value == NULL || clazz == NULL) {
        if (key != NULL) (*env)->DeleteLocalRef(env, key);
        if (value != NULL) (*env)->DeleteLocalRef(env, value);
        if (clazz != NULL) (*env)->DeleteLocalRef(env, clazz);
        return;
    }

    jmethodID method = (*env)->GetStaticMethodID(env, clazz, "setProperty", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
    if (method != NULL) {
        jobject oldValue = (*env)->CallStaticObjectMethod(env, clazz, method, key, value);
        if (oldValue != NULL) {
            (*env)->DeleteLocalRef(env, oldValue);
        }
    }
    (*env)->DeleteLocalRef(env, clazz);
    (*env)->DeleteLocalRef(env, value);
    (*env)->DeleteLocalRef(env, key);
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
    NSString *renderer = environment[@"POJAV_RENDERER_BACKEND"];
    if (renderer.length == 0) {
        renderer = requestedRenderer;
    }
    if (renderer.length == 0) {
        renderer = @"auto";
    }
    BOOL useMetalCraft = [requestedRenderer isEqualToString:@ RENDERER_NAME_METALCRAFT];
    BOOL isAuto = [renderer isEqualToString:@"auto"];
    if (useMetalCraft) {
        if (renderer.length == 0 ||
            [renderer isEqualToString:@"auto"] ||
            [renderer isEqualToString:requestedRenderer]) {
            // Keep MetalCraft's backend as MetalCraft itself for better Sodium compatibility
            // instead of forcing it to use MobileGlues as backend
            renderer = @ RENDERER_NAME_METALCRAFT;
        }
        setenv("POJAV_RENDERER_BACKEND", renderer.UTF8String, 1);
        set_gl_bridge_tbl();
    } else if (isAuto || [renderer isEqualToString:@ RENDERER_NAME_GL4ES]) {
        // At this point, if renderer is still auto (unspecified major version), pick gl4es
        renderer = @ RENDERER_NAME_GL4ES;
        setenv("POJAV_RENDERER_BACKEND", renderer.UTF8String, 1);
        set_gl_bridge_tbl();
    } else if ([renderer isEqualToString:@ RENDERER_NAME_MOBILEGLUES]) {
        renderer = @ RENDERER_NAME_MOBILEGLUES;
        setenv("POJAV_RENDERER_BACKEND", renderer.UTF8String, 1);
        set_gl_bridge_tbl();
    } else if ([renderer isEqualToString:@ RENDERER_NAME_KRYPTON_WRAPPER]) {
        renderer = @ RENDERER_NAME_KRYPTON_WRAPPER;
        setenv("POJAV_RENDERER_BACKEND", renderer.UTF8String, 1);
        set_gl_bridge_tbl();
    } else if ([renderer isEqualToString:@ RENDERER_NAME_MTL_ANGLE]) {
        setenv("POJAV_RENDERER_BACKEND", renderer.UTF8String, 1);
        set_gl_bridge_tbl();
    } else if ([renderer hasPrefix:@"libOSMesa"]) {
        setenv("GALLIUM_DRIVER","zink",1);
        setenv("POJAV_RENDERER_BACKEND", renderer.UTF8String, 1);
        set_osm_bridge_tbl();
    }
    if (!useMetalCraft) {
        JNI_LWJGL_changeRenderer(renderer.UTF8String);
    }
    // Preload renderer library
    dlopen([NSString stringWithFormat:@"@rpath/%@", renderer].UTF8String, RTLD_GLOBAL);
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
                JNI_LWJGL_changeRenderer(RENDERER_NAME_GL4ES);
                break;
            // case 4: use Zink?
            default:
                setenv("AMETHYST_RENDERER", RENDERER_NAME_MOBILEGLUES, 1);
                setenv("POJAV_RENDERER_BACKEND", RENDERER_NAME_MOBILEGLUES, 1);
                JNI_LWJGL_changeRenderer(RENDERER_NAME_MOBILEGLUES);
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
