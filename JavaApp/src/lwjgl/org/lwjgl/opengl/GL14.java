package org.lwjgl.opengl;

import net.kdt.pojavlaunch.render.MetalCraftGLInterceptor;

public class GL14 extends GL14C {
    private GL14() {}

    public static void glBlendEquation(int mode) {
        MetalCraftGLInterceptor.setBlendEquationSeparate(mode, mode);
        GL14C.glBlendEquation(mode);
    }

    public static void glBlendFuncSeparate(int srcRGB, int dstRGB, int srcAlpha, int dstAlpha) {
        MetalCraftGLInterceptor.setBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
        GL14C.glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
    }
}
