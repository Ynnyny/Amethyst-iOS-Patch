package org.lwjgl.opengl;

import java.nio.ByteBuffer;
import java.nio.IntBuffer;

public class ARBMultiDrawIndirect {
    private ARBMultiDrawIndirect() {}

    public static void nglMultiDrawArraysIndirect(int mode, long indirect, int drawCount,
            int stride) {
        GL43C.nglMultiDrawArraysIndirect(mode, indirect, drawCount, stride);
    }

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

    public static void nglMultiDrawElementsIndirect(int mode, int type, long indirect,
            int drawCount, int stride) {
        GL43C.nglMultiDrawElementsIndirect(mode, type, indirect, drawCount, stride);
    }

    public static void glMultiDrawElementsIndirect(int mode, int type, ByteBuffer indirect,
            int drawCount, int stride) {
        GL43C.glMultiDrawElementsIndirect(mode, type, indirect, drawCount, stride);
    }

    public static void glMultiDrawElementsIndirect(int mode, int type, long indirect,
            int drawCount, int stride) {
        GL43C.glMultiDrawElementsIndirect(mode, type, indirect, drawCount, stride);
    }

    public static void glMultiDrawElementsIndirect(int mode, int type, IntBuffer indirect,
            int drawCount, int stride) {
        GL43C.glMultiDrawElementsIndirect(mode, type, indirect, drawCount, stride);
    }

    public static void glMultiDrawArraysIndirect(int mode, int[] indirect, int drawCount,
            int stride) {
        GL43C.glMultiDrawArraysIndirect(mode, indirect, drawCount, stride);
    }

    public static void glMultiDrawElementsIndirect(int mode, int type, int[] indirect,
            int drawCount, int stride) {
        GL43C.glMultiDrawElementsIndirect(mode, type, indirect, drawCount, stride);
    }
}
