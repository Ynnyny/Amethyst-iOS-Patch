package net.kdt.pojavlaunch.render;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicBoolean;

public final class MetalCraftGLInterceptor {
    private static final int GL_BLEND = 0x0BE2;
    private static final int GL_DEPTH_TEST = 0x0B71;

    private static final int GL_ZERO = 0;
    private static final int GL_ONE = 1;
    private static final int GL_SRC_COLOR = 0x0300;
    private static final int GL_ONE_MINUS_SRC_COLOR = 0x0301;
    private static final int GL_SRC_ALPHA = 0x0302;
    private static final int GL_ONE_MINUS_SRC_ALPHA = 0x0303;
    private static final int GL_DST_ALPHA = 0x0304;
    private static final int GL_ONE_MINUS_DST_ALPHA = 0x0305;
    private static final int GL_DST_COLOR = 0x0306;
    private static final int GL_ONE_MINUS_DST_COLOR = 0x0307;

    private static final int GL_NEVER = 0x0200;
    private static final int GL_LESS = 0x0201;
    private static final int GL_EQUAL = 0x0202;
    private static final int GL_LEQUAL = 0x0203;
    private static final int GL_GREATER = 0x0204;
    private static final int GL_NOTEQUAL = 0x0205;
    private static final int GL_GEQUAL = 0x0206;
    private static final int GL_ALWAYS = 0x0207;

    private static final int GL_POINTS = 0x0000;
    private static final int GL_LINES = 0x0001;
    private static final int GL_LINE_STRIP = 0x0003;
    private static final int GL_TRIANGLES = 0x0004;
    private static final int GL_TRIANGLE_STRIP = 0x0005;

    private static final int GL_FUNC_ADD = 0x8006;
    private static final int GL_MIN = 0x8007;
    private static final int GL_MAX = 0x8008;
    private static final int GL_FUNC_SUBTRACT = 0x800A;
    private static final int GL_FUNC_REVERSE_SUBTRACT = 0x800B;

    private static final int GL_VERTEX_SHADER = 0x8B31;
    private static final int GL_FRAGMENT_SHADER = 0x8B30;
    private static final int GL_COMPUTE_SHADER = 0x91B9;

    private static final int GL_RGBA16F = 0x881A;
    private static final int GL_DEPTH_COMPONENT32F = 0x8CAC;
    private static final int GL_DEPTH24_STENCIL8 = 0x88F0;

    private static final boolean ACTIVE =
            Boolean.getBoolean("pojav.renderer.metalcraft") && MetalCraftBridge.isAvailable();
    private static final AtomicBoolean SHUTDOWN_HOOK_INSTALLED = new AtomicBoolean(false);
    private static final Object LOCK = new Object();
    private static final Map<Long, ProgramBinding> PROGRAMS = new HashMap<Long, ProgramBinding>();

    private static long currentProgramId;
    private static boolean blendEnabled;
    private static int srcColorFactor = MetalCraftBridge.BLEND_FACTOR_ONE;
    private static int dstColorFactor = MetalCraftBridge.BLEND_FACTOR_ZERO;
    private static int srcAlphaFactor = MetalCraftBridge.BLEND_FACTOR_ONE;
    private static int dstAlphaFactor = MetalCraftBridge.BLEND_FACTOR_ZERO;
    private static int colorBlendOp = MetalCraftBridge.BLEND_OP_ADD;
    private static int alphaBlendOp = MetalCraftBridge.BLEND_OP_ADD;
    private static int colorWriteMask = 0x0F;
    private static boolean depthTestEnabled;
    private static boolean depthWriteEnabled = true;
    private static int depthCompareOp = MetalCraftBridge.COMPARE_OP_LESS_EQUAL;
    private static int colorFormat = MetalCraftBridge.PIXEL_FORMAT_BGRA8_UNORM;
    private static int depthFormat = MetalCraftBridge.PIXEL_FORMAT_DEPTH32_FLOAT;
    private static int sampleCount = 1;
    private static int topology = MetalCraftBridge.TOPOLOGY_TRIANGLES;

    private MetalCraftGLInterceptor() {}

    public static boolean bootstrap() {
        if (!ACTIVE) {
            return false;
        }

        reset();
        if (SHUTDOWN_HOOK_INSTALLED.compareAndSet(false, true)) {
            Runtime.getRuntime().addShutdownHook(new Thread(new Runnable() {
                @Override
                public void run() {
                    MetalCraftGLInterceptor.shutdown();
                }
            }, "MetalCraft-Shutdown"));
        }
        System.setProperty("pojav.renderer.metalcraft.interceptor", "true");
        return true;
    }

    public static boolean isActive() {
        return ACTIVE;
    }

    public static void reset() {
        if (!ACTIVE) {
            return;
        }

        synchronized (LOCK) {
            PROGRAMS.clear();
            currentProgramId = 0L;
            blendEnabled = false;
            srcColorFactor = MetalCraftBridge.BLEND_FACTOR_ONE;
            dstColorFactor = MetalCraftBridge.BLEND_FACTOR_ZERO;
            srcAlphaFactor = MetalCraftBridge.BLEND_FACTOR_ONE;
            dstAlphaFactor = MetalCraftBridge.BLEND_FACTOR_ZERO;
            colorBlendOp = MetalCraftBridge.BLEND_OP_ADD;
            alphaBlendOp = MetalCraftBridge.BLEND_OP_ADD;
            colorWriteMask = 0x0F;
            depthTestEnabled = false;
            depthWriteEnabled = true;
            depthCompareOp = MetalCraftBridge.COMPARE_OP_LESS_EQUAL;
            colorFormat = MetalCraftBridge.PIXEL_FORMAT_BGRA8_UNORM;
            depthFormat = MetalCraftBridge.PIXEL_FORMAT_DEPTH32_FLOAT;
            sampleCount = 1;
            topology = MetalCraftBridge.TOPOLOGY_TRIANGLES;
            MetalCraftBridge.resetStateTracker();
        }
    }

    public static void shutdown() {
        if (!ACTIVE) {
            return;
        }

        synchronized (LOCK) {
            PROGRAMS.clear();
            currentProgramId = 0L;
            MetalCraftBridge.shutdown();
        }
    }

    public static boolean registerShaderSource(long shaderId, int glShaderType, String source,
            boolean flipVertexY) {
        if (!ACTIVE || shaderId == 0L || source == null || source.length() == 0) {
            return false;
        }

        int stage = toShaderStage(glShaderType);
        if (stage < 0) {
            return false;
        }

        return MetalCraftBridge.registerShaderSource(shaderId, stage, source, flipVertexY);
    }

    public static void registerProgram(long programId, long vertexShaderId, long fragmentShaderId,
            long vertexLayoutId) {
        if (!ACTIVE || programId == 0L) {
            return;
        }

        synchronized (LOCK) {
            PROGRAMS.put(Long.valueOf(programId),
                    new ProgramBinding(vertexShaderId, fragmentShaderId, vertexLayoutId));
            if (currentProgramId == programId) {
                syncProgramBindingLocked();
            }
        }
    }

    public static void unregisterProgram(long programId) {
        if (!ACTIVE || programId == 0L) {
            return;
        }

        synchronized (LOCK) {
            PROGRAMS.remove(Long.valueOf(programId));
            if (currentProgramId == programId) {
                currentProgramId = 0L;
                syncProgramBindingLocked();
            }
        }
    }

    public static void useProgram(long programId) {
        if (!ACTIVE) {
            return;
        }

        synchronized (LOCK) {
            currentProgramId = programId;
            syncProgramBindingLocked();
        }
    }

    public static void setCapability(int capability, boolean enabled) {
        if (!ACTIVE) {
            return;
        }

        synchronized (LOCK) {
            if (capability == GL_BLEND) {
                blendEnabled = enabled;
                syncBlendLocked();
            } else if (capability == GL_DEPTH_TEST) {
                depthTestEnabled = enabled;
                syncDepthLocked();
            }
        }
    }

    public static void setBlendFuncSeparate(int srcColor, int dstColor, int srcAlpha,
            int dstAlpha) {
        if (!ACTIVE) {
            return;
        }

        synchronized (LOCK) {
            srcColorFactor = toBlendFactor(srcColor);
            dstColorFactor = toBlendFactor(dstColor);
            srcAlphaFactor = toBlendFactor(srcAlpha);
            dstAlphaFactor = toBlendFactor(dstAlpha);
            syncBlendLocked();
        }
    }

    public static void setBlendEquationSeparate(int colorOp, int alphaOp) {
        if (!ACTIVE) {
            return;
        }

        synchronized (LOCK) {
            colorBlendOp = toBlendOp(colorOp);
            alphaBlendOp = toBlendOp(alphaOp);
            syncBlendLocked();
        }
    }

    public static void setColorMask(boolean red, boolean green, boolean blue, boolean alpha) {
        if (!ACTIVE) {
            return;
        }

        synchronized (LOCK) {
            colorWriteMask = 0;
            if (red) colorWriteMask |= 0x1;
            if (green) colorWriteMask |= 0x2;
            if (blue) colorWriteMask |= 0x4;
            if (alpha) colorWriteMask |= 0x8;
            syncBlendLocked();
        }
    }

    public static void setDepthMask(boolean enabled) {
        if (!ACTIVE) {
            return;
        }

        synchronized (LOCK) {
            depthWriteEnabled = enabled;
            syncDepthLocked();
        }
    }

    public static void setDepthFunc(int func) {
        if (!ACTIVE) {
            return;
        }

        synchronized (LOCK) {
            depthCompareOp = toCompareOp(func);
            syncDepthLocked();
        }
    }

    public static void setRenderPassFromGL(int glColorFormat, int glDepthFormat,
            int renderSampleCount, int drawMode) {
        if (!ACTIVE) {
            return;
        }

        synchronized (LOCK) {
            colorFormat = toPixelFormat(glColorFormat);
            depthFormat = toPixelFormat(glDepthFormat);
            sampleCount = renderSampleCount <= 0 ? 1 : renderSampleCount;
            topology = toTopology(drawMode);
            MetalCraftBridge.setRenderPassState(colorFormat, depthFormat, sampleCount, topology);
        }
    }

    public static void beginFrame() {
        if (ACTIVE) {
            MetalCraftBridge.beginFrame();
        }
    }

    public static void endFrame() {
        if (ACTIVE) {
            MetalCraftBridge.endFrame();
        }
    }

    public static void drawArrays(int drawMode, int first, int count, int instanceCount) {
        if (!ACTIVE) {
            return;
        }

        synchronized (LOCK) {
            topology = toTopology(drawMode);
            MetalCraftBridge.setRenderPassState(colorFormat, depthFormat, sampleCount, topology);
            syncProgramBindingLocked();
            MetalCraftBridge.draw(topology, first, count, normalizeInstanceCount(instanceCount));
        }
    }

    public static void drawIndexed(int drawMode, int indexCount, long indexBufferOffset,
            int instanceCount) {
        if (!ACTIVE) {
            return;
        }

        synchronized (LOCK) {
            topology = toTopology(drawMode);
            MetalCraftBridge.setRenderPassState(colorFormat, depthFormat, sampleCount, topology);
            syncProgramBindingLocked();
            MetalCraftBridge.drawIndexed(topology, indexCount, indexBufferOffset,
                    normalizeInstanceCount(instanceCount));
        }
    }

    public static boolean drawMultiArraysIndirect(int drawMode, long commandsPtr, int drawCount,
            int stride) {
        if (!ACTIVE) {
            return false;
        }

        synchronized (LOCK) {
            topology = toTopology(drawMode);
            MetalCraftBridge.setRenderPassState(colorFormat, depthFormat, sampleCount, topology);
            syncProgramBindingLocked();
            return MetalCraftBridge.drawMulti(topology, commandsPtr, drawCount, stride);
        }
    }

    private static int normalizeInstanceCount(int instanceCount) {
        return instanceCount <= 0 ? 1 : instanceCount;
    }

    private static void syncBlendLocked() {
        MetalCraftBridge.setBlendState(blendEnabled, srcColorFactor, dstColorFactor,
                srcAlphaFactor, dstAlphaFactor, colorBlendOp, alphaBlendOp, colorWriteMask);
    }

    private static void syncDepthLocked() {
        MetalCraftBridge.setDepthState(depthTestEnabled, depthWriteEnabled, depthCompareOp);
    }

    private static void syncProgramBindingLocked() {
        ProgramBinding binding = PROGRAMS.get(Long.valueOf(currentProgramId));
        if (binding == null) {
            MetalCraftBridge.bindShaders(0L, 0L, 0L);
            return;
        }

        MetalCraftBridge.bindShaders(binding.vertexShaderId, binding.fragmentShaderId,
                binding.vertexLayoutId);
    }

    private static int toShaderStage(int glShaderType) {
        switch (glShaderType) {
            case GL_VERTEX_SHADER:
                return MetalCraftBridge.SHADER_STAGE_VERTEX;
            case GL_FRAGMENT_SHADER:
                return MetalCraftBridge.SHADER_STAGE_FRAGMENT;
            case GL_COMPUTE_SHADER:
                return MetalCraftBridge.SHADER_STAGE_COMPUTE;
            default:
                return -1;
        }
    }

    private static int toBlendFactor(int glBlendFactor) {
        switch (glBlendFactor) {
            case GL_ZERO:
                return MetalCraftBridge.BLEND_FACTOR_ZERO;
            case GL_ONE:
                return MetalCraftBridge.BLEND_FACTOR_ONE;
            case GL_SRC_COLOR:
                return MetalCraftBridge.BLEND_FACTOR_SRC_COLOR;
            case GL_ONE_MINUS_SRC_COLOR:
                return MetalCraftBridge.BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
            case GL_SRC_ALPHA:
                return MetalCraftBridge.BLEND_FACTOR_SRC_ALPHA;
            case GL_ONE_MINUS_SRC_ALPHA:
                return MetalCraftBridge.BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            case GL_DST_COLOR:
                return MetalCraftBridge.BLEND_FACTOR_DST_COLOR;
            case GL_ONE_MINUS_DST_COLOR:
                return MetalCraftBridge.BLEND_FACTOR_ONE_MINUS_DST_COLOR;
            case GL_DST_ALPHA:
                return MetalCraftBridge.BLEND_FACTOR_DST_ALPHA;
            case GL_ONE_MINUS_DST_ALPHA:
                return MetalCraftBridge.BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
            default:
                return MetalCraftBridge.BLEND_FACTOR_ONE;
        }
    }

    private static int toBlendOp(int glBlendOp) {
        switch (glBlendOp) {
            case GL_FUNC_ADD:
                return MetalCraftBridge.BLEND_OP_ADD;
            case GL_FUNC_SUBTRACT:
                return MetalCraftBridge.BLEND_OP_SUBTRACT;
            case GL_FUNC_REVERSE_SUBTRACT:
                return MetalCraftBridge.BLEND_OP_REVERSE_SUBTRACT;
            case GL_MIN:
                return MetalCraftBridge.BLEND_OP_MIN;
            case GL_MAX:
                return MetalCraftBridge.BLEND_OP_MAX;
            default:
                return MetalCraftBridge.BLEND_OP_ADD;
        }
    }

    private static int toCompareOp(int glCompareFunc) {
        switch (glCompareFunc) {
            case GL_NEVER:
                return MetalCraftBridge.COMPARE_OP_NEVER;
            case GL_LESS:
                return MetalCraftBridge.COMPARE_OP_LESS;
            case GL_LEQUAL:
                return MetalCraftBridge.COMPARE_OP_LESS_EQUAL;
            case GL_EQUAL:
                return MetalCraftBridge.COMPARE_OP_EQUAL;
            case GL_NOTEQUAL:
                return MetalCraftBridge.COMPARE_OP_NOT_EQUAL;
            case GL_GREATER:
                return MetalCraftBridge.COMPARE_OP_GREATER;
            case GL_GEQUAL:
                return MetalCraftBridge.COMPARE_OP_GREATER_EQUAL;
            case GL_ALWAYS:
                return MetalCraftBridge.COMPARE_OP_ALWAYS;
            default:
                return MetalCraftBridge.COMPARE_OP_LESS_EQUAL;
        }
    }

    private static int toTopology(int glDrawMode) {
        switch (glDrawMode) {
            case GL_POINTS:
                return MetalCraftBridge.TOPOLOGY_POINTS;
            case GL_LINES:
                return MetalCraftBridge.TOPOLOGY_LINES;
            case GL_LINE_STRIP:
                return MetalCraftBridge.TOPOLOGY_LINE_STRIP;
            case GL_TRIANGLE_STRIP:
                return MetalCraftBridge.TOPOLOGY_TRIANGLE_STRIP;
            case GL_TRIANGLES:
            default:
                return MetalCraftBridge.TOPOLOGY_TRIANGLES;
        }
    }

    private static int toPixelFormat(int glFormat) {
        switch (glFormat) {
            case GL_RGBA16F:
                return MetalCraftBridge.PIXEL_FORMAT_RGBA16_FLOAT;
            case GL_DEPTH_COMPONENT32F:
                return MetalCraftBridge.PIXEL_FORMAT_DEPTH32_FLOAT;
            case GL_DEPTH24_STENCIL8:
                return MetalCraftBridge.PIXEL_FORMAT_DEPTH24_STENCIL8;
            case 0:
                return MetalCraftBridge.PIXEL_FORMAT_INVALID;
            default:
                return MetalCraftBridge.PIXEL_FORMAT_BGRA8_UNORM;
        }
    }

    private static final class ProgramBinding {
        final long vertexShaderId;
        final long fragmentShaderId;
        final long vertexLayoutId;

        ProgramBinding(long vertexShaderId, long fragmentShaderId, long vertexLayoutId) {
            this.vertexShaderId = vertexShaderId;
            this.fragmentShaderId = fragmentShaderId;
            this.vertexLayoutId = vertexLayoutId;
        }
    }
}
