package net.kdt.pojavlaunch.render;

public final class MetalCraftBridge {
    private MetalCraftBridge() {}

    public static native void nResetStateTracker();
    public static native void nSetBlendState(boolean enabled, int srcColor, int dstColor,
            int srcAlpha, int dstAlpha, int colorOp, int alphaOp, int writeMask);
    public static native void nSetDepthState(boolean testEnabled, boolean writeEnabled,
            int compareOp);
    public static native void nBindShaders(long vertexShaderId, long fragmentShaderId,
            long vertexLayoutId);
    public static native void nSetRenderPassState(int colorFormat, int depthFormat,
            int sampleCount, int topology);
    public static native long nCurrentPipelineHash();
}
