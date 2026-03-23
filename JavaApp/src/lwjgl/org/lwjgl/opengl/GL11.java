package org.lwjgl.opengl;

import java.nio.ByteBuffer;
import java.nio.IntBuffer;

import net.kdt.pojavlaunch.render.MetalCraftGLInterceptor;

public class GL11 extends GL11C {
    private GL11() {}

    public static void glEnable(int cap) {
        MetalCraftGLInterceptor.setCapability(cap, true);
        GL11C.glEnable(cap);
    }

    public static void glDisable(int cap) {
        MetalCraftGLInterceptor.setCapability(cap, false);
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
        MetalCraftGLInterceptor.defineTexture(target, internalformat, 1);
        GL11C.glTexImage2D(target, level, internalformat, width, height, border, format, type,
                pixels);
    }

    public static void glTexImage2D(int target, int level, int internalformat, int width,
            int height, int border, int format, int type, long pixels) {
        MetalCraftGLInterceptor.defineTexture(target, internalformat, 1);
        GL11C.glTexImage2D(target, level, internalformat, width, height, border, format, type,
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
}
