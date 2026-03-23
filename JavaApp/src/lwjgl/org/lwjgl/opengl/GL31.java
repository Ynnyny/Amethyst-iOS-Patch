package org.lwjgl.opengl;

import net.kdt.pojavlaunch.render.MetalCraftGLInterceptor;

public class GL31 extends GL31C {
    private GL31() {}

    public static void glDrawArraysInstanced(int mode, int first, int count, int instanceCount) {
        MetalCraftGLInterceptor.drawArrays(mode, first, count, instanceCount);
        GL31C.glDrawArraysInstanced(mode, first, count, instanceCount);
    }

    public static void glDrawElementsInstanced(int mode, int count, int type, long indices,
            int instanceCount) {
        MetalCraftGLInterceptor.drawIndexed(mode, count, indices, instanceCount);
        GL31C.glDrawElementsInstanced(mode, count, type, indices, instanceCount);
    }
}
