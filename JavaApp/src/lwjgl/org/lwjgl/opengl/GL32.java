package org.lwjgl.opengl;

import net.kdt.pojavlaunch.render.MetalCraftGLInterceptor;

public class GL32 extends GL32C {
    private GL32() {}

    public static void glDrawElementsBaseVertex(int mode, int count, int type, long indices,
            int baseVertex) {
        MetalCraftGLInterceptor.drawIndexed(mode, count, indices, 1);
        GL32C.glDrawElementsBaseVertex(mode, count, type, indices, baseVertex);
    }

    public static void glDrawElementsInstancedBaseVertex(int mode, int count, int type,
            long indices, int instanceCount, int baseVertex) {
        MetalCraftGLInterceptor.drawIndexed(mode, count, indices, instanceCount);
        GL32C.glDrawElementsInstancedBaseVertex(mode, count, type, indices, instanceCount,
                baseVertex);
    }

    public static void glTexImage2DMultisample(int target, int samples, int internalformat,
            int width, int height, boolean fixedSampleLocations) {
        MetalCraftGLInterceptor.defineTexture(target, internalformat, samples);
        GL32C.glTexImage2DMultisample(target, samples, internalformat, width, height,
                fixedSampleLocations);
    }

    public static void glFramebufferTexture(int target, int attachment, int texture, int level) {
        GL32C.glFramebufferTexture(target, attachment, texture, level);
        MetalCraftGLInterceptor.attachFramebufferTexture(target, attachment,
                MetalCraftOpenGLHooks.asUnsignedId(texture));
    }
}
