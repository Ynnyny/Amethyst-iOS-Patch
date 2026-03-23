package org.lwjgl.opengl;

import java.nio.IntBuffer;

import net.kdt.pojavlaunch.render.MetalCraftGLInterceptor;

public class GL30 extends GL30C {
    private GL30() {}

    public static void glBindRenderbuffer(int target, int renderbuffer) {
        MetalCraftGLInterceptor.bindRenderbuffer(target,
                MetalCraftOpenGLHooks.asUnsignedId(renderbuffer));
        GL30C.glBindRenderbuffer(target, renderbuffer);
    }

    public static void glRenderbufferStorage(int target, int internalformat, int width,
            int height) {
        MetalCraftGLInterceptor.defineRenderbuffer(internalformat, 1);
        GL30C.glRenderbufferStorage(target, internalformat, width, height);
    }

    public static void glRenderbufferStorageMultisample(int target, int samples,
            int internalformat, int width, int height) {
        MetalCraftGLInterceptor.defineRenderbuffer(internalformat, samples);
        GL30C.glRenderbufferStorageMultisample(target, samples, internalformat, width, height);
    }

    public static void glBindFramebuffer(int target, int framebuffer) {
        MetalCraftGLInterceptor.bindFramebuffer(target,
                MetalCraftOpenGLHooks.asUnsignedId(framebuffer));
        GL30C.glBindFramebuffer(target, framebuffer);
    }

    public static void glFramebufferTexture2D(int target, int attachment, int textarget,
            int texture, int level) {
        GL30C.glFramebufferTexture2D(target, attachment, textarget, texture, level);
        MetalCraftGLInterceptor.attachFramebufferTexture(target, attachment,
                MetalCraftOpenGLHooks.asUnsignedId(texture));
    }

    public static void glFramebufferRenderbuffer(int target, int attachment,
            int renderbuffertarget, int renderbuffer) {
        GL30C.glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
        MetalCraftGLInterceptor.attachFramebufferRenderbuffer(target, attachment,
                MetalCraftOpenGLHooks.asUnsignedId(renderbuffer));
    }

    public static void glDeleteRenderbuffers(int renderbuffer) {
        MetalCraftGLInterceptor.deleteRenderbuffer(
                MetalCraftOpenGLHooks.asUnsignedId(renderbuffer));
        GL30C.glDeleteRenderbuffers(renderbuffer);
    }

    public static void glDeleteRenderbuffers(IntBuffer renderbuffers) {
        IntBuffer copy = renderbuffers == null ? null : renderbuffers.duplicate();
        if (copy != null) {
            while (copy.hasRemaining()) {
                MetalCraftGLInterceptor.deleteRenderbuffer(
                        MetalCraftOpenGLHooks.asUnsignedId(copy.get()));
            }
        }
        GL30C.glDeleteRenderbuffers(renderbuffers);
    }

    public static void glDeleteRenderbuffers(int[] renderbuffers) {
        if (renderbuffers != null) {
            for (int i = 0; i < renderbuffers.length; ++i) {
                MetalCraftGLInterceptor.deleteRenderbuffer(
                        MetalCraftOpenGLHooks.asUnsignedId(renderbuffers[i]));
            }
        }
        GL30C.glDeleteRenderbuffers(renderbuffers);
    }

    public static void glDeleteFramebuffers(int framebuffer) {
        MetalCraftGLInterceptor.deleteFramebuffer(
                MetalCraftOpenGLHooks.asUnsignedId(framebuffer));
        GL30C.glDeleteFramebuffers(framebuffer);
    }

    public static void glDeleteFramebuffers(IntBuffer framebuffers) {
        IntBuffer copy = framebuffers == null ? null : framebuffers.duplicate();
        if (copy != null) {
            while (copy.hasRemaining()) {
                MetalCraftGLInterceptor.deleteFramebuffer(
                        MetalCraftOpenGLHooks.asUnsignedId(copy.get()));
            }
        }
        GL30C.glDeleteFramebuffers(framebuffers);
    }

    public static void glDeleteFramebuffers(int[] framebuffers) {
        if (framebuffers != null) {
            for (int i = 0; i < framebuffers.length; ++i) {
                MetalCraftGLInterceptor.deleteFramebuffer(
                        MetalCraftOpenGLHooks.asUnsignedId(framebuffers[i]));
            }
        }
        GL30C.glDeleteFramebuffers(framebuffers);
    }
}
