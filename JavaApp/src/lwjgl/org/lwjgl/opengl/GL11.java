package org.lwjgl.opengl;

import java.nio.ByteBuffer;
import java.nio.IntBuffer;

import net.kdt.pojavlaunch.render.MetalCraftGLInterceptor;

public class GL11 extends GL11C {
    private static final String BACKEND_PROPERTY = "pojav.renderer.backend";
    private static final String RENDERER_GL4ES = "libgl4es_114.dylib";
    private static final String RENDERER_ANGLE = "libtinygl4angle.dylib";
    private static final String RENDERER_MOBILEGLUES = "libmobileglues.dylib";
    private static final String RENDERER_KRYPTON = "libng_gl4es.dylib";

    private static final int GL_TEXTURE_CUBE_MAP_SEAMLESS = 0x884F;
    private static final int GL_PROGRAM_POINT_SIZE = 0x8642;
    private static final int GL_BGRA = 0x80E1;
    private static final int GL_UNSIGNED_INT_8_8_8_8 = 0x8035;
    private static final int GL_UNSIGNED_INT_8_8_8_8_REV = 0x8367;
    private static final int GL_DEPTH_COMPONENT = 0x1902;
    private static final int GL_DEPTH_COMPONENT24 = 0x81A6;
    private static final int GL_DEPTH_COMPONENT32 = 0x81A7;
    private static final int GL_DEPTH_COMPONENT32F = 0x8CAC;
    private static final int GL_FLOAT = 0x1406;
    private static final int GL_UNSIGNED_INT = 0x1405;

    private GL11() {}

    private static boolean shouldIgnoreCapability(int cap) {
        return cap == GL_TEXTURE_CUBE_MAP_SEAMLESS || cap == GL_PROGRAM_POINT_SIZE;
    }

    private static boolean isPackedBgraType(int type) {
        return type == GL_UNSIGNED_INT_8_8_8_8 || type == GL_UNSIGNED_INT_8_8_8_8_REV;
    }

    private static boolean isDepth32LikeInternalFormat(int internalformat) {
        return internalformat == GL_DEPTH_COMPONENT32 || internalformat == GL_DEPTH_COMPONENT32F;
    }

    private static int[] normalizeTexImage2DParams(int internalformat, int format, int type) {
        if (format == GL_BGRA && isPackedBgraType(type)) {
            format = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
        }
        if (isDepth32LikeInternalFormat(internalformat) && format == GL_DEPTH_COMPONENT) {
            internalformat = GL_DEPTH_COMPONENT24;
            type = GL_UNSIGNED_INT;
        }
        return new int[] {internalformat, format, type};
    }

    private static String getMetalCraftVersionString() {
        String backend = System.getProperty(BACKEND_PROPERTY, "");
        if (RENDERER_GL4ES.equals(backend)) {
            return "2.1 MetalCraft / gl4es";
        }
        if (RENDERER_ANGLE.equals(backend)) {
            return "3.2 MetalCraft / ANGLE";
        }
        if (RENDERER_MOBILEGLUES.equals(backend)) {
            return "4.0 MetalCraft / MobileGlues";
        }
        if (RENDERER_KRYPTON.equals(backend)) {
            return "4.1 MetalCraft / Krypton Wrapper";
        }
        return "4.1 MetalCraft";
    }

    private static String getMetalCraftRendererString() {
        String backend = System.getProperty(BACKEND_PROPERTY, "");
        if (backend == null || backend.length() == 0) {
            return "MetalCraft Native Pipeline (Apple GPU)";
        }
        if (RENDERER_GL4ES.equals(backend)) {
            return "MetalCraft Native Pipeline (gl4es backend)";
        }
        if (RENDERER_ANGLE.equals(backend)) {
            return "MetalCraft Native Pipeline (ANGLE backend)";
        }
        if (RENDERER_MOBILEGLUES.equals(backend)) {
            return "MetalCraft Native Pipeline (MobileGlues backend)";
        }
        if (RENDERER_KRYPTON.equals(backend)) {
            return "MetalCraft Native Pipeline (Krypton Wrapper backend)";
        }
        return "MetalCraft Native Pipeline (" + backend + ")";
    }

    public static void glEnable(int cap) {
        MetalCraftGLInterceptor.setCapability(cap, true);
        if (shouldIgnoreCapability(cap)) {
            return;
        }
        GL11C.glEnable(cap);
    }

    public static void glDisable(int cap) {
        MetalCraftGLInterceptor.setCapability(cap, false);
        if (shouldIgnoreCapability(cap)) {
            return;
        }
        GL11C.glDisable(cap);
    }

    public static void glBindTexture(int target, int texture) {
        MetalCraftGLInterceptor.bindTexture(target, MetalCraftOpenGLHooks.asUnsignedId(texture));
        GL11C.glBindTexture(target, texture);
    }

    public static void glBlendFunc(int sfactor, int dfactor) {
        MetalCraftGLInterceptor.setBlendFuncSeparate(sfactor, dfactor, sfactor, dfactor);
        GL11C.glBlendFunc(sfactor, dfactor);
    }

    public static void glColorMask(boolean red, boolean green, boolean blue, boolean alpha) {
        MetalCraftGLInterceptor.setColorMask(red, green, blue, alpha);
        GL11C.glColorMask(red, green, blue, alpha);
    }

    public static void glDepthFunc(int func) {
        MetalCraftGLInterceptor.setDepthFunc(func);
        GL11C.glDepthFunc(func);
    }

    public static void glDepthMask(boolean flag) {
        MetalCraftGLInterceptor.setDepthMask(flag);
        GL11C.glDepthMask(flag);
    }

    public static void glDrawArrays(int mode, int first, int count) {
        MetalCraftGLInterceptor.drawArrays(mode, first, count, 1);
        GL11C.glDrawArrays(mode, first, count);
    }

    public static void glDrawElements(int mode, int count, int type, long indices) {
        MetalCraftGLInterceptor.drawIndexed(mode, count, indices, 1);
        GL11C.glDrawElements(mode, count, type, indices);
    }

    public static void glTexImage2D(int target, int level, int internalformat, int width,
            int height, int border, int format, int type, ByteBuffer pixels) {
        int[] normalized = normalizeTexImage2DParams(internalformat, format, type);
        internalformat = normalized[0];
        format = normalized[1];
        type = normalized[2];
        MetalCraftGLInterceptor.defineTexture(target, internalformat, 1);
        GL11C.glTexImage2D(target, level, internalformat, width, height, border, format, type,
                pixels);
    }

    public static void glTexImage2D(int target, int level, int internalformat, int width,
            int height, int border, int format, int type, long pixels) {
        int[] normalized = normalizeTexImage2DParams(internalformat, format, type);
        internalformat = normalized[0];
        format = normalized[1];
        type = normalized[2];
        MetalCraftGLInterceptor.defineTexture(target, internalformat, 1);
        GL11C.glTexImage2D(target, level, internalformat, width, height, border, format, type,
                pixels);
    }

    public static void glTexSubImage2D(int target, int level, int xoffset, int yoffset, int width,
            int height, int format, int type, ByteBuffer pixels) {
        int[] normalized = normalizeTexImage2DParams(0, format, type);
        format = normalized[1];
        type = normalized[2];
        GL11C.glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type,
                pixels);
    }

    public static void glTexSubImage2D(int target, int level, int xoffset, int yoffset, int width,
            int height, int format, int type, long pixels) {
        int[] normalized = normalizeTexImage2DParams(0, format, type);
        format = normalized[1];
        type = normalized[2];
        GL11C.glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type,
                pixels);
    }

    public static void glDeleteTextures(int texture) {
        MetalCraftGLInterceptor.deleteTexture(MetalCraftOpenGLHooks.asUnsignedId(texture));
        GL11C.glDeleteTextures(texture);
    }

    public static void glDeleteTextures(IntBuffer textures) {
        IntBuffer copy = textures == null ? null : textures.duplicate();
        if (copy != null) {
            while (copy.hasRemaining()) {
                MetalCraftGLInterceptor.deleteTexture(
                        MetalCraftOpenGLHooks.asUnsignedId(copy.get()));
            }
        }
        GL11C.glDeleteTextures(textures);
    }

    public static void glDeleteTextures(int[] textures) {
        if (textures != null) {
            for (int i = 0; i < textures.length; ++i) {
                MetalCraftGLInterceptor.deleteTexture(
                        MetalCraftOpenGLHooks.asUnsignedId(textures[i]));
            }
        }
        GL11C.glDeleteTextures(textures);
    }

    public static String glGetString(int name) {
        if (MetalCraftGLInterceptor.isActive()) {
            if (name == 0x1F01) { // GL_RENDERER
                return getMetalCraftRendererString();
            }
            if (name == 0x1F00) { // GL_VENDOR
                return "Amethyst-iOS";
            }
            if (name == 0x1F02) { // GL_VERSION
                return getMetalCraftVersionString();
            }
        }
        return GL11C.glGetString(name);
    }
}
