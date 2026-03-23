package org.lwjgl.opengl;

import java.nio.ByteBuffer;
import java.nio.IntBuffer;

public class GL43 extends GL43C {
    private GL43() {}

    public static void glMultiDrawArraysIndirect(int mode, ByteBuffer indirect, int drawCount,
            int stride) {
        MetalCraftOpenGLHooks.recordMultiDrawArraysIndirect(mode, indirect, drawCount, stride);
        GL43C.glMultiDrawArraysIndirect(mode, indirect, drawCount, stride);
    }

    public static void glMultiDrawArraysIndirect(int mode, long indirect, int drawCount,
            int stride) {
        MetalCraftOpenGLHooks.recordMultiDrawArraysIndirect(mode, indirect, drawCount, stride);
        GL43C.glMultiDrawArraysIndirect(mode, indirect, drawCount, stride);
    }

    public static void glMultiDrawArraysIndirect(int mode, IntBuffer indirect, int drawCount,
            int stride) {
        MetalCraftOpenGLHooks.recordMultiDrawArraysIndirect(mode, indirect, drawCount, stride);
        GL43C.glMultiDrawArraysIndirect(mode, indirect, drawCount, stride);
    }
}
