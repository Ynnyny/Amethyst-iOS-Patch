package net.kdt.pojavlaunch.render;

public final class MetalCraftBridge {
    public static final String RENDERER_NAME = "libMetalCraft.dylib";
    public static final int SHADER_STAGE_VERTEX = 0;
    public static final int SHADER_STAGE_FRAGMENT = 1;
    public static final int SHADER_STAGE_COMPUTE = 2;

    private static final boolean AVAILABLE = loadNativeLibrary();

    private MetalCraftBridge() {}

    private static boolean loadNativeLibrary() {
        try {
            System.loadLibrary("MetalCraft");
            return true;
        } catch (UnsatisfiedLinkError ignored) {
            return false;
        }
    }

    public static boolean isAvailable() {
        return AVAILABLE;
    }

    public static boolean bootstrapRequestedRenderer() {
        boolean requested = RENDERER_NAME.equals(System.getProperty("pojav.renderer.requested"));
        if (!requested) {
            return false;
        }

        System.setProperty("pojav.renderer.metalcraft.active", Boolean.toString(AVAILABLE));
        return AVAILABLE;
    }

    public static void resetStateTracker() {
        if (AVAILABLE) nResetStateTracker();
    }

    public static void setBlendState(boolean enabled, int srcColor, int dstColor,
            int srcAlpha, int dstAlpha, int colorOp, int alphaOp, int writeMask) {
        if (AVAILABLE) {
            nSetBlendState(enabled, srcColor, dstColor, srcAlpha, dstAlpha,
                    colorOp, alphaOp, writeMask);
        }
    }

    public static void setDepthState(boolean testEnabled, boolean writeEnabled,
            int compareOp) {
        if (AVAILABLE) nSetDepthState(testEnabled, writeEnabled, compareOp);
    }

    public static void bindShaders(long vertexShaderId, long fragmentShaderId,
            long vertexLayoutId) {
        if (AVAILABLE) nBindShaders(vertexShaderId, fragmentShaderId, vertexLayoutId);
    }

    public static void setRenderPassState(int colorFormat, int depthFormat,
            int sampleCount, int topology) {
        if (AVAILABLE) nSetRenderPassState(colorFormat, depthFormat, sampleCount, topology);
    }

    public static boolean registerShaderSource(long shaderId, int stage, String glslSource,
            boolean flipVertexY) {
        return AVAILABLE && glslSource != null
                && nRegisterShaderSource(shaderId, stage, glslSource, flipVertexY);
    }

    public static long acquireCurrentPipeline() {
        return AVAILABLE ? nAcquireCurrentPipeline() : 0L;
    }

    public static long currentPipelineHash() {
        return AVAILABLE ? nCurrentPipelineHash() : 0L;
    }

    public static void beginFrame() {
        if (AVAILABLE) nBeginFrame();
    }

    public static void endFrame() {
        if (AVAILABLE) nEndFrame();
    }

    public static void draw(int topology, int vertexStart, int vertexCount, int instanceCount) {
        if (AVAILABLE) nDraw(topology, vertexStart, vertexCount, instanceCount);
    }

    public static void drawIndexed(int topology, int indexCount, long indexBufferOffset,
            int instanceCount) {
        if (AVAILABLE) nDrawIndexed(topology, indexCount, indexBufferOffset, instanceCount);
    }

    public static boolean drawMulti(int topology, long commandsPtr, int drawCount, int stride) {
        return AVAILABLE && nDrawMulti(topology, commandsPtr, drawCount, stride);
    }

    public static long getDrawCallCount() {
        return AVAILABLE ? nGetDrawCallCount() : 0L;
    }

    public static long createTexture(int width, int height, int format, boolean mipmapped) {
        return AVAILABLE ? nCreateTexture(width, height, format, mipmapped) : 0L;
    }

    public static boolean uploadTexture(long textureId, int x, int y, int w, int h,
            long dataPtr, long bytesPerRow) {
        return AVAILABLE && nUploadTexture(textureId, x, y, w, h, dataPtr, bytesPerRow);
    }

    private static native void nResetStateTracker();
    private static native void nSetBlendState(boolean enabled, int srcColor, int dstColor,
            int srcAlpha, int dstAlpha, int colorOp, int alphaOp, int writeMask);
    private static native void nSetDepthState(boolean testEnabled, boolean writeEnabled,
            int compareOp);
    private static native void nBindShaders(long vertexShaderId, long fragmentShaderId,
            long vertexLayoutId);
    private static native void nSetRenderPassState(int colorFormat, int depthFormat,
            int sampleCount, int topology);
    private static native boolean nRegisterShaderSource(long shaderId, int stage,
            String glslSource, boolean flipVertexY);
    private static native long nAcquireCurrentPipeline();
    private static native long nCurrentPipelineHash();
    private static native void nBeginFrame();
    private static native void nEndFrame();
    private static native void nDraw(int topology, int vertexStart, int vertexCount,
            int instanceCount);
    private static native void nDrawIndexed(int topology, int indexCount,
            long indexBufferOffset, int instanceCount);
    private static native boolean nDrawMulti(int topology, long commandsPtr, int drawCount,
            int stride);
    private static native long nGetDrawCallCount();
    private static native long nCreateTexture(int width, int height, int format,
            boolean mipmapped);
    private static native boolean nUploadTexture(long textureId, int x, int y, int w, int h,
            long dataPtr, long bytesPerRow);
}
