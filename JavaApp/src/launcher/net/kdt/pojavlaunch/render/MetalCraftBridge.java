package net.kdt.pojavlaunch.render;

public final class MetalCraftBridge {
    public static final String RENDERER_NAME = "libMetalCraft.dylib";
    public static final int BUFFER_USAGE_VERTEX = 0;
    public static final int BUFFER_USAGE_INDEX = 1;
    public static final int BUFFER_USAGE_UNIFORM = 2;
    public static final int BUFFER_USAGE_UPLOAD = 3;
    public static final int BUFFER_USAGE_INDIRECT = 4;

    public static final int COMPARE_OP_NEVER = 0;
    public static final int COMPARE_OP_LESS = 1;
    public static final int COMPARE_OP_LESS_EQUAL = 2;
    public static final int COMPARE_OP_EQUAL = 3;
    public static final int COMPARE_OP_NOT_EQUAL = 4;
    public static final int COMPARE_OP_GREATER = 5;
    public static final int COMPARE_OP_GREATER_EQUAL = 6;
    public static final int COMPARE_OP_ALWAYS = 7;

    public static final int BLEND_FACTOR_ZERO = 0;
    public static final int BLEND_FACTOR_ONE = 1;
    public static final int BLEND_FACTOR_SRC_COLOR = 2;
    public static final int BLEND_FACTOR_ONE_MINUS_SRC_COLOR = 3;
    public static final int BLEND_FACTOR_SRC_ALPHA = 4;
    public static final int BLEND_FACTOR_ONE_MINUS_SRC_ALPHA = 5;
    public static final int BLEND_FACTOR_DST_COLOR = 6;
    public static final int BLEND_FACTOR_ONE_MINUS_DST_COLOR = 7;
    public static final int BLEND_FACTOR_DST_ALPHA = 8;
    public static final int BLEND_FACTOR_ONE_MINUS_DST_ALPHA = 9;

    public static final int BLEND_OP_ADD = 0;
    public static final int BLEND_OP_SUBTRACT = 1;
    public static final int BLEND_OP_REVERSE_SUBTRACT = 2;
    public static final int BLEND_OP_MIN = 3;
    public static final int BLEND_OP_MAX = 4;

    public static final int TOPOLOGY_TRIANGLES = 0;
    public static final int TOPOLOGY_TRIANGLE_STRIP = 1;
    public static final int TOPOLOGY_LINES = 2;
    public static final int TOPOLOGY_LINE_STRIP = 3;
    public static final int TOPOLOGY_POINTS = 4;

    public static final int PIXEL_FORMAT_INVALID = 0;
    public static final int PIXEL_FORMAT_BGRA8_UNORM = 80;
    public static final int PIXEL_FORMAT_RGBA16_FLOAT = 112;
    public static final int PIXEL_FORMAT_DEPTH32_FLOAT = 252;
    public static final int PIXEL_FORMAT_DEPTH24_STENCIL8 = 255;

    public static final int SHADER_STAGE_VERTEX = 0;
    public static final int SHADER_STAGE_FRAGMENT = 1;
    public static final int SHADER_STAGE_COMPUTE = 2;

    private static final boolean AVAILABLE = loadNativeLibrary();

    private MetalCraftBridge() {}

    private static boolean loadNativeLibrary() {
        try {
            System.loadLibrary("MetalCraft");
            return nIsSupported();
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

    public static void shutdown() {
        if (AVAILABLE) nShutdown();
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

    public static boolean destroyBuffer(long bufferId) {
        return AVAILABLE && nDestroyBuffer(bufferId);
    }

    public static long createBuffer(long size, int usage, boolean cpuVisible) {
        return AVAILABLE ? nCreateBuffer(size, usage, cpuVisible) : 0L;
    }

    public static boolean updateBuffer(long bufferId, long offset, long dataPtr, long length) {
        return AVAILABLE && nUpdateBuffer(bufferId, offset, dataPtr, length);
    }

    public static boolean destroyTexture(long textureId) {
        return AVAILABLE && nDestroyTexture(textureId);
    }

    public static long createTexture(int width, int height, int format, boolean mipmapped) {
        return AVAILABLE ? nCreateTexture(width, height, format, mipmapped) : 0L;
    }

    public static boolean uploadTexture(long textureId, int x, int y, int w, int h,
            long dataPtr, long bytesPerRow) {
        return AVAILABLE && nUploadTexture(textureId, x, y, w, h, dataPtr, bytesPerRow);
    }

    private static native void nResetStateTracker();
    private static native void nShutdown();
    private static native boolean nIsSupported();
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
    private static native boolean nDestroyBuffer(long bufferId);
    private static native long nCreateBuffer(long size, int usage, boolean cpuVisible);
    private static native boolean nUpdateBuffer(long bufferId, long offset, long dataPtr,
            long length);
    private static native boolean nDestroyTexture(long textureId);
    private static native long nCreateTexture(int width, int height, int format,
            boolean mipmapped);
    private static native boolean nUploadTexture(long textureId, int x, int y, int w, int h,
            long dataPtr, long bytesPerRow);
}
