package org.lwjgl.opengl;

import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;

import net.kdt.pojavlaunch.render.MetalCraftGLInterceptor;

import org.lwjgl.system.MemoryUtil;

final class MetalCraftOpenGLHooks {
    private static final int GL_DRAW_INDIRECT_BUFFER_BINDING = 0x8F43;

    private MetalCraftOpenGLHooks() {}

    static long asUnsignedId(int value) {
        return value & 0xFFFFFFFFL;
    }

    static long directAddress(Buffer buffer) {
        if (buffer == null || !buffer.isDirect()) {
            return 0L;
        }
        return MemoryUtil.memAddress(buffer);
    }

    static String concat(CharSequence... values) {
        if (values == null || values.length == 0) {
            return "";
        }

        StringBuilder builder = new StringBuilder();
        for (int i = 0; i < values.length; ++i) {
            CharSequence value = values[i];
            if (value != null) {
                builder.append(value);
            }
        }
        return builder.toString();
    }

    static void recordMultiDrawArraysIndirect(int mode, ByteBuffer indirect, int drawCount,
            int stride) {
        long pointer = directAddress(indirect);
        if (pointer > 0L) {
            MetalCraftGLInterceptor.drawMultiArraysIndirect(mode, pointer, drawCount, stride);
        }
    }

    static void recordMultiDrawArraysIndirect(int mode, IntBuffer indirect, int drawCount,
            int stride) {
        long pointer = directAddress(indirect);
        if (pointer > 0L) {
            MetalCraftGLInterceptor.drawMultiArraysIndirect(mode, pointer, drawCount, stride);
        }
    }

    static void recordMultiDrawArraysIndirect(int mode, long indirect, int drawCount, int stride) {
        if (indirect <= 0L || GL11C.glGetInteger(GL_DRAW_INDIRECT_BUFFER_BINDING) != 0) {
            return;
        }

        MetalCraftGLInterceptor.drawMultiArraysIndirect(mode, indirect, drawCount, stride);
    }
}
