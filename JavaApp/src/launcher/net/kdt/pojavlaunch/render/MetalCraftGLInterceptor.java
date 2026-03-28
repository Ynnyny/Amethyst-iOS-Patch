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
    private static final int GL_CONSTANT_COLOR = 0x8001;
    private static final int GL_ONE_MINUS_CONSTANT_COLOR = 0x8002;
    private static final int GL_CONSTANT_ALPHA = 0x8003;
    private static final int GL_ONE_MINUS_CONSTANT_ALPHA = 0x8004;

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

    private static final int GL_TEXTURE_2D = 0x0DE1;
    private static final int GL_TEXTURE_2D_MULTISAMPLE = 0x9100;
    private static final int GL_RENDERBUFFER = 0x8D41;
    private static final int GL_FRAMEBUFFER = 0x8D40;
    private static final int GL_READ_FRAMEBUFFER = 0x8CA8;
    private static final int GL_DRAW_FRAMEBUFFER = 0x8CA9;
    private static final int GL_COLOR_ATTACHMENT0 = 0x8CE0;
    private static final int GL_DEPTH_ATTACHMENT = 0x8D00;
    private static final int GL_STENCIL_ATTACHMENT = 0x8D20;
    private static final int GL_DEPTH_STENCIL_ATTACHMENT = 0x821A;

    private static final int GL_RGBA8 = 0x8058;
    private static final int GL_SRGB8_ALPHA8 = 0x8C43;
    private static final int GL_RGBA16F = 0x881A;
    private static final int GL_DEPTH_COMPONENT32F = 0x8CAC;
    private static final int GL_DEPTH24_STENCIL8 = 0x88F0;

    private static final boolean ACTIVE =
            Boolean.getBoolean("pojav.renderer.metalcraft") && MetalCraftBridge.isAvailable();
    private static final AtomicBoolean SHUTDOWN_HOOK_INSTALLED = new AtomicBoolean(false);
    private static final Object LOCK = new Object();

    private static final Map<Long, ShaderState> SHADERS = new HashMap<Long, ShaderState>();
    private static final Map<Long, ProgramState> PROGRAMS = new HashMap<Long, ProgramState>();
    private static final Map<Long, TextureState> TEXTURES = new HashMap<Long, TextureState>();
    private static final Map<Long, RenderbufferState> RENDERBUFFERS =
            new HashMap<Long, RenderbufferState>();
    private static final Map<Long, FramebufferState> FRAMEBUFFERS =
            new HashMap<Long, FramebufferState>();
    private static final Map<Integer, Long> BOUND_TEXTURES = new HashMap<Integer, Long>();

    private static long currentProgramId;
    private static long boundRenderbufferId;
    private static long drawFramebufferId;
    private static long readFramebufferId;
    private static boolean frameActive;

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

    private static int defaultColorFormat = MetalCraftBridge.PIXEL_FORMAT_BGRA8_UNORM;
    private static int defaultDepthFormat = MetalCraftBridge.PIXEL_FORMAT_DEPTH32_FLOAT;
    private static int defaultSampleCount = 1;
    private static int colorFormat = defaultColorFormat;
    private static int depthFormat = defaultDepthFormat;
    private static int sampleCount = defaultSampleCount;
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
            if (frameActive) {
                MetalCraftBridge.endFrame();
            }

            SHADERS.clear();
            PROGRAMS.clear();
            TEXTURES.clear();
            RENDERBUFFERS.clear();
            FRAMEBUFFERS.clear();
            BOUND_TEXTURES.clear();

            currentProgramId = 0L;
            boundRenderbufferId = 0L;
            drawFramebufferId = 0L;
            readFramebufferId = 0L;
            frameActive = false;

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

            defaultColorFormat = MetalCraftBridge.PIXEL_FORMAT_BGRA8_UNORM;
            defaultDepthFormat = MetalCraftBridge.PIXEL_FORMAT_DEPTH32_FLOAT;
            defaultSampleCount = 1;
            colorFormat = defaultColorFormat;
            depthFormat = defaultDepthFormat;
            sampleCount = defaultSampleCount;
            topology = MetalCraftBridge.TOPOLOGY_TRIANGLES;

            MetalCraftBridge.resetStateTracker();
            syncBlendLocked();
            syncDepthLocked();
            syncRenderPassStateLocked();
            syncProgramBindingLocked();
        }
    }

    public static void shutdown() {
        if (!ACTIVE) {
            return;
        }

        synchronized (LOCK) {
            if (frameActive) {
                MetalCraftBridge.endFrame();
                frameActive = false;
            }
            SHADERS.clear();
            PROGRAMS.clear();
            TEXTURES.clear();
            RENDERBUFFERS.clear();
            FRAMEBUFFERS.clear();
            BOUND_TEXTURES.clear();
            currentProgramId = 0L;
            boundRenderbufferId = 0L;
            drawFramebufferId = 0L;
            readFramebufferId = 0L;
            MetalCraftBridge.shutdown();
        }
    }

    public static void onShaderCreated(long shaderId, int glShaderType) {
        if (!ACTIVE || shaderId == 0L) {
            return;
        }

        synchronized (LOCK) {
            ShaderState shader = ensureShaderStateLocked(shaderId);
            shader.glShaderType = glShaderType;
        }
    }

    public static boolean registerShaderSource(long shaderId, int glShaderType, String source,
            boolean flipVertexY) {
        if (!ACTIVE || shaderId == 0L || source == null || source.length() == 0) {
            return false;
        }

        synchronized (LOCK) {
            ShaderState shader = ensureShaderStateLocked(shaderId);
            shader.glShaderType = glShaderType;
            shader.source = source;
            shader.flipVertexY = flipVertexY;
            return registerShaderSourceLocked(shaderId, shader);
        }
    }

    public static boolean registerShaderSource(long shaderId, String source, boolean flipVertexY) {
        if (!ACTIVE || shaderId == 0L || source == null || source.length() == 0) {
            return false;
        }

        synchronized (LOCK) {
            ShaderState shader = ensureShaderStateLocked(shaderId);
            shader.source = source;
            shader.flipVertexY = flipVertexY;
            return registerShaderSourceLocked(shaderId, shader);
        }
    }

    public static void onShaderDeleted(long shaderId) {
        if (!ACTIVE || shaderId == 0L) {
            return;
        }

        synchronized (LOCK) {
            SHADERS.remove(Long.valueOf(shaderId));
            for (ProgramState program : PROGRAMS.values()) {
                if (program.vertexShaderId == shaderId) {
                    program.vertexShaderId = 0L;
                    program.linked = false;
                }
                if (program.fragmentShaderId == shaderId) {
                    program.fragmentShaderId = 0L;
                    program.linked = false;
                }
            }
            syncProgramBindingLocked();
        }
    }

    public static boolean hasShaderSource(long shaderId) {
        if (!ACTIVE || shaderId == 0L) {
            return false;
        }

        synchronized (LOCK) {
            ShaderState shader = SHADERS.get(Long.valueOf(shaderId));
            return shader != null && shader.source != null && shader.source.length() > 0;
        }
    }

    public static int getShaderType(long shaderId) {
        if (!ACTIVE || shaderId == 0L) {
            return 0;
        }

        synchronized (LOCK) {
            ShaderState shader = SHADERS.get(Long.valueOf(shaderId));
            return shader == null ? 0 : shader.glShaderType;
        }
    }

    public static void onShaderAttached(long programId, long shaderId) {
        if (!ACTIVE || programId == 0L || shaderId == 0L) {
            return;
        }

        synchronized (LOCK) {
            ProgramState program = ensureProgramStateLocked(programId);
            ShaderState shader = SHADERS.get(Long.valueOf(shaderId));
            if (shader == null) {
                return;
            }

            if (shader.glShaderType == GL_VERTEX_SHADER) {
                program.vertexShaderId = shaderId;
                program.linked = false;
            } else if (shader.glShaderType == GL_FRAGMENT_SHADER) {
                program.fragmentShaderId = shaderId;
                program.linked = false;
            }
        }
    }

    public static void onShaderDetached(long programId, long shaderId) {
        if (!ACTIVE || programId == 0L || shaderId == 0L) {
            return;
        }

        synchronized (LOCK) {
            ProgramState program = PROGRAMS.get(Long.valueOf(programId));
            if (program == null) {
                return;
            }

            if (program.vertexShaderId == shaderId) {
                program.vertexShaderId = 0L;
                program.linked = false;
            }
            if (program.fragmentShaderId == shaderId) {
                program.fragmentShaderId = 0L;
                program.linked = false;
            }
            if (currentProgramId == programId) {
                syncProgramBindingLocked();
            }
        }
    }

    public static boolean canLinkProgram(long programId) {
        if (!ACTIVE || programId == 0L) {
            return false;
        }

        synchronized (LOCK) {
            ProgramState program = PROGRAMS.get(Long.valueOf(programId));
            return program != null && hasLinkedStagesLocked(program);
        }
    }

    public static boolean isProgramLinked(long programId) {
        if (!ACTIVE || programId == 0L) {
            return false;
        }

        synchronized (LOCK) {
            ProgramState program = PROGRAMS.get(Long.valueOf(programId));
            return program != null && program.linked;
        }
    }

    public static void onProgramLinked(long programId) {
        if (!ACTIVE || programId == 0L) {
            return;
        }

        synchronized (LOCK) {
            ProgramState program = ensureProgramStateLocked(programId);
            program.linked = hasLinkedStagesLocked(program);
            if (currentProgramId == programId) {
                syncProgramBindingLocked();
            }
        }
    }

    public static void onProgramLinkFailed(long programId) {
        if (!ACTIVE || programId == 0L) {
            return;
        }

        synchronized (LOCK) {
            ProgramState program = ensureProgramStateLocked(programId);
            program.linked = false;
            if (currentProgramId == programId) {
                syncProgramBindingLocked();
            }
        }
    }

    public static void registerProgram(long programId, long vertexShaderId, long fragmentShaderId,
            long vertexLayoutId) {
        if (!ACTIVE || programId == 0L) {
            return;
        }

        synchronized (LOCK) {
            ProgramState program = ensureProgramStateLocked(programId);
            program.vertexShaderId = vertexShaderId;
            program.fragmentShaderId = fragmentShaderId;
            program.vertexLayoutId = vertexLayoutId;
            program.linked = true;
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

    public static void onProgramDeleted(long programId) {
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
            defaultColorFormat = toPixelFormat(glColorFormat);
            defaultDepthFormat = toPixelFormat(glDepthFormat);
            defaultSampleCount = renderSampleCount <= 0 ? 1 : renderSampleCount;
            topology = toTopology(drawMode);
            syncRenderPassStateLocked();
        }
    }

    public static void bindTexture(int target, long textureId) {
        if (!ACTIVE || !isTrackableTextureTarget(target)) {
            return;
        }

        synchronized (LOCK) {
            BOUND_TEXTURES.put(Integer.valueOf(target), Long.valueOf(textureId));
        }
    }

    public static void defineTexture(int target, int glInternalFormat, int renderSampleCount) {
        if (!ACTIVE || !isTrackableTextureTarget(target)) {
            return;
        }

        synchronized (LOCK) {
            long textureId = getBoundTextureIdLocked(target);
            if (textureId == 0L) {
                return;
            }

            TextureState texture = ensureTextureStateLocked(textureId);
            texture.format = toPixelFormat(glInternalFormat);
            texture.sampleCount = renderSampleCount <= 0 ? 1 : renderSampleCount;
            syncRenderPassStateLocked();
        }
    }

    public static void deleteTexture(long textureId) {
        if (!ACTIVE || textureId == 0L) {
            return;
        }

        synchronized (LOCK) {
            TEXTURES.remove(Long.valueOf(textureId));
            removeBoundTextureLocked(textureId);
            detachTextureFromFramebuffersLocked(textureId);
            syncRenderPassStateLocked();
        }
    }

    public static void bindRenderbuffer(int target, long renderbufferId) {
        if (!ACTIVE || target != GL_RENDERBUFFER) {
            return;
        }

        synchronized (LOCK) {
            boundRenderbufferId = renderbufferId;
        }
    }

    public static void defineRenderbuffer(int glInternalFormat, int renderSampleCount) {
        if (!ACTIVE) {
            return;
        }

        synchronized (LOCK) {
            if (boundRenderbufferId == 0L) {
                return;
            }

            RenderbufferState renderbuffer = ensureRenderbufferStateLocked(boundRenderbufferId);
            renderbuffer.format = toPixelFormat(glInternalFormat);
            renderbuffer.sampleCount = renderSampleCount <= 0 ? 1 : renderSampleCount;
            syncRenderPassStateLocked();
        }
    }

    public static void deleteRenderbuffer(long renderbufferId) {
        if (!ACTIVE || renderbufferId == 0L) {
            return;
        }

        synchronized (LOCK) {
            RENDERBUFFERS.remove(Long.valueOf(renderbufferId));
            if (boundRenderbufferId == renderbufferId) {
                boundRenderbufferId = 0L;
            }
            detachRenderbufferFromFramebuffersLocked(renderbufferId);
            syncRenderPassStateLocked();
        }
    }

    public static void bindFramebuffer(int target, long framebufferId) {
        if (!ACTIVE) {
            return;
        }

        synchronized (LOCK) {
            if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER) {
                drawFramebufferId = framebufferId;
            }
            if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER) {
                readFramebufferId = framebufferId;
            }
            syncRenderPassStateLocked();
        }
    }

    public static void attachFramebufferTexture(int target, int attachment, long textureId) {
        if (!ACTIVE) {
            return;
        }

        synchronized (LOCK) {
            long framebufferId = resolveFramebufferBindingLocked(target);
            FramebufferState framebuffer = ensureFramebufferStateLocked(framebufferId);
            setFramebufferTextureAttachmentLocked(framebuffer, attachment, textureId);
            syncRenderPassStateLocked();
        }
    }

    public static void attachFramebufferRenderbuffer(int target, int attachment, long renderbufferId) {
        if (!ACTIVE) {
            return;
        }

        synchronized (LOCK) {
            long framebufferId = resolveFramebufferBindingLocked(target);
            FramebufferState framebuffer = ensureFramebufferStateLocked(framebufferId);
            setFramebufferRenderbufferAttachmentLocked(framebuffer, attachment, renderbufferId);
            syncRenderPassStateLocked();
        }
    }

    public static void deleteFramebuffer(long framebufferId) {
        if (!ACTIVE || framebufferId == 0L) {
            return;
        }

        synchronized (LOCK) {
            FRAMEBUFFERS.remove(Long.valueOf(framebufferId));
            if (drawFramebufferId == framebufferId) {
                drawFramebufferId = 0L;
            }
            if (readFramebufferId == framebufferId) {
                readFramebufferId = 0L;
            }
            syncRenderPassStateLocked();
        }
    }

    public static void beginFrame() {
        if (!ACTIVE) {
            return;
        }

        synchronized (LOCK) {
            ensureFrameLocked();
        }
    }

    public static void endFrame() {
        if (!ACTIVE) {
            return;
        }

        synchronized (LOCK) {
            if (frameActive) {
                MetalCraftBridge.endFrame();
                frameActive = false;
            }
        }
    }

    public static void drawArrays(int drawMode, int first, int count, int instanceCount) {
        if (!ACTIVE) {
            return;
        }

        synchronized (LOCK) {
            topology = toTopology(drawMode);
            syncProgramBindingLocked();
            ensureFrameLocked();
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
            syncProgramBindingLocked();
            ensureFrameLocked();
            MetalCraftBridge.drawIndexed(topology, indexCount, indexBufferOffset,
                    normalizeInstanceCount(instanceCount));
        }
    }

    public static boolean drawMultiArraysIndirect(int drawMode, long commandsPtr, int drawCount,
            int stride) {
        if (!ACTIVE || commandsPtr <= 0L || drawCount <= 0) {
            return false;
        }

        synchronized (LOCK) {
            topology = toTopology(drawMode);
            syncProgramBindingLocked();
            ensureFrameLocked();
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
        ProgramState binding = PROGRAMS.get(Long.valueOf(currentProgramId));
        if (binding == null || !binding.linked) {
            MetalCraftBridge.bindShaders(0L, 0L, 0L);
            return;
        }

        MetalCraftBridge.bindShaders(binding.vertexShaderId, binding.fragmentShaderId,
                binding.vertexLayoutId);
    }

    private static void ensureFrameLocked() {
        syncRenderPassStateLocked();
        if (!frameActive) {
            MetalCraftBridge.beginFrame();
            frameActive = true;
        }
    }

    private static void syncRenderPassStateLocked() {
        int resolvedColorFormat = defaultColorFormat;
        int resolvedDepthFormat = defaultDepthFormat;
        int resolvedSampleCount = defaultSampleCount <= 0 ? 1 : defaultSampleCount;

        if (drawFramebufferId != 0L) {
            FramebufferState framebuffer = FRAMEBUFFERS.get(Long.valueOf(drawFramebufferId));
            if (framebuffer != null) {
                int trackedColor = resolveColorFormatLocked(framebuffer);
                int trackedDepth = resolveDepthFormatLocked(framebuffer);
                int trackedSamples = resolveSampleCountLocked(framebuffer);

                if (trackedColor != MetalCraftBridge.PIXEL_FORMAT_INVALID) {
                    resolvedColorFormat = trackedColor;
                }

                if (framebuffer.hasDepthAttachment()) {
                    resolvedDepthFormat = trackedDepth;
                } else {
                    resolvedDepthFormat = MetalCraftBridge.PIXEL_FORMAT_INVALID;
                }

                if (trackedSamples > 0) {
                    resolvedSampleCount = trackedSamples;
                }
            }
        }

        colorFormat = resolvedColorFormat;
        depthFormat = resolvedDepthFormat;
        sampleCount = resolvedSampleCount;
        MetalCraftBridge.setRenderPassState(colorFormat, depthFormat, sampleCount, topology);
    }

    private static boolean registerShaderSourceLocked(long shaderId, ShaderState shader) {
        int stage = toShaderStage(shader.glShaderType);
        if (stage < 0 || shader.source == null || shader.source.length() == 0) {
            return false;
        }

        return MetalCraftBridge.registerShaderSource(shaderId, stage, shader.source,
                shader.flipVertexY);
    }

    private static boolean hasLinkedStagesLocked(ProgramState program) {
        return hasStageLocked(program.vertexShaderId, GL_VERTEX_SHADER)
                && hasStageLocked(program.fragmentShaderId, GL_FRAGMENT_SHADER);
    }

    private static boolean hasStageLocked(long shaderId, int expectedType) {
        if (shaderId == 0L) {
            return false;
        }

        ShaderState shader = SHADERS.get(Long.valueOf(shaderId));
        return shader != null && shader.glShaderType == expectedType
                && shader.source != null && shader.source.length() > 0;
    }

    private static ShaderState ensureShaderStateLocked(long shaderId) {
        Long key = Long.valueOf(shaderId);
        ShaderState shader = SHADERS.get(key);
        if (shader == null) {
            shader = new ShaderState();
            SHADERS.put(key, shader);
        }
        return shader;
    }

    private static ProgramState ensureProgramStateLocked(long programId) {
        Long key = Long.valueOf(programId);
        ProgramState program = PROGRAMS.get(key);
        if (program == null) {
            program = new ProgramState();
            PROGRAMS.put(key, program);
        }
        return program;
    }

    private static TextureState ensureTextureStateLocked(long textureId) {
        Long key = Long.valueOf(textureId);
        TextureState texture = TEXTURES.get(key);
        if (texture == null) {
            texture = new TextureState();
            TEXTURES.put(key, texture);
        }
        return texture;
    }

    private static RenderbufferState ensureRenderbufferStateLocked(long renderbufferId) {
        Long key = Long.valueOf(renderbufferId);
        RenderbufferState renderbuffer = RENDERBUFFERS.get(key);
        if (renderbuffer == null) {
            renderbuffer = new RenderbufferState();
            RENDERBUFFERS.put(key, renderbuffer);
        }
        return renderbuffer;
    }

    private static FramebufferState ensureFramebufferStateLocked(long framebufferId) {
        Long key = Long.valueOf(framebufferId);
        FramebufferState framebuffer = FRAMEBUFFERS.get(key);
        if (framebuffer == null) {
            framebuffer = new FramebufferState();
            FRAMEBUFFERS.put(key, framebuffer);
        }
        return framebuffer;
    }

    private static long getBoundTextureIdLocked(int target) {
        Long value = BOUND_TEXTURES.get(Integer.valueOf(target));
        return value == null ? 0L : value.longValue();
    }

    private static void removeBoundTextureLocked(long textureId) {
        Integer[] targets = BOUND_TEXTURES.keySet().toArray(new Integer[0]);
        for (int i = 0; i < targets.length; ++i) {
            Integer target = targets[i];
            Long boundTexture = BOUND_TEXTURES.get(target);
            if (boundTexture != null && boundTexture.longValue() == textureId) {
                BOUND_TEXTURES.remove(target);
            }
        }
    }

    private static long resolveFramebufferBindingLocked(int target) {
        if (target == GL_READ_FRAMEBUFFER) {
            return readFramebufferId;
        }
        return drawFramebufferId;
    }

    private static void setFramebufferTextureAttachmentLocked(FramebufferState framebuffer,
            int attachment, long textureId) {
        if (isColorAttachment(attachment)) {
            framebuffer.colorTextureId = textureId;
            framebuffer.colorRenderbufferId = 0L;
            return;
        }

        if (attachment == GL_DEPTH_ATTACHMENT || attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
            framebuffer.depthTextureId = textureId;
            framebuffer.depthRenderbufferId = 0L;
        }
    }

    private static void setFramebufferRenderbufferAttachmentLocked(FramebufferState framebuffer,
            int attachment, long renderbufferId) {
        if (isColorAttachment(attachment)) {
            framebuffer.colorRenderbufferId = renderbufferId;
            framebuffer.colorTextureId = 0L;
            return;
        }

        if (attachment == GL_DEPTH_ATTACHMENT || attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
            framebuffer.depthRenderbufferId = renderbufferId;
            framebuffer.depthTextureId = 0L;
        }
    }

    private static void detachTextureFromFramebuffersLocked(long textureId) {
        for (FramebufferState framebuffer : FRAMEBUFFERS.values()) {
            if (framebuffer.colorTextureId == textureId) {
                framebuffer.colorTextureId = 0L;
            }
            if (framebuffer.depthTextureId == textureId) {
                framebuffer.depthTextureId = 0L;
            }
        }
    }

    private static void detachRenderbufferFromFramebuffersLocked(long renderbufferId) {
        for (FramebufferState framebuffer : FRAMEBUFFERS.values()) {
            if (framebuffer.colorRenderbufferId == renderbufferId) {
                framebuffer.colorRenderbufferId = 0L;
            }
            if (framebuffer.depthRenderbufferId == renderbufferId) {
                framebuffer.depthRenderbufferId = 0L;
            }
        }
    }

    private static int resolveColorFormatLocked(FramebufferState framebuffer) {
        if (framebuffer.colorTextureId != 0L) {
            TextureState texture = TEXTURES.get(Long.valueOf(framebuffer.colorTextureId));
            if (texture != null) {
                return texture.format;
            }
        }

        if (framebuffer.colorRenderbufferId != 0L) {
            RenderbufferState renderbuffer =
                    RENDERBUFFERS.get(Long.valueOf(framebuffer.colorRenderbufferId));
            if (renderbuffer != null) {
                return renderbuffer.format;
            }
        }

        return MetalCraftBridge.PIXEL_FORMAT_INVALID;
    }

    private static int resolveDepthFormatLocked(FramebufferState framebuffer) {
        if (framebuffer.depthTextureId != 0L) {
            TextureState texture = TEXTURES.get(Long.valueOf(framebuffer.depthTextureId));
            if (texture != null) {
                return texture.format;
            }
        }

        if (framebuffer.depthRenderbufferId != 0L) {
            RenderbufferState renderbuffer =
                    RENDERBUFFERS.get(Long.valueOf(framebuffer.depthRenderbufferId));
            if (renderbuffer != null) {
                return renderbuffer.format;
            }
        }

        return MetalCraftBridge.PIXEL_FORMAT_INVALID;
    }

    private static int resolveSampleCountLocked(FramebufferState framebuffer) {
        if (framebuffer.colorTextureId != 0L) {
            TextureState texture = TEXTURES.get(Long.valueOf(framebuffer.colorTextureId));
            if (texture != null && texture.sampleCount > 0) {
                return texture.sampleCount;
            }
        }

        if (framebuffer.colorRenderbufferId != 0L) {
            RenderbufferState renderbuffer =
                    RENDERBUFFERS.get(Long.valueOf(framebuffer.colorRenderbufferId));
            if (renderbuffer != null && renderbuffer.sampleCount > 0) {
                return renderbuffer.sampleCount;
            }
        }

        if (framebuffer.depthTextureId != 0L) {
            TextureState texture = TEXTURES.get(Long.valueOf(framebuffer.depthTextureId));
            if (texture != null && texture.sampleCount > 0) {
                return texture.sampleCount;
            }
        }

        if (framebuffer.depthRenderbufferId != 0L) {
            RenderbufferState renderbuffer =
                    RENDERBUFFERS.get(Long.valueOf(framebuffer.depthRenderbufferId));
            if (renderbuffer != null && renderbuffer.sampleCount > 0) {
                return renderbuffer.sampleCount;
            }
        }

        return defaultSampleCount;
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
            case GL_CONSTANT_COLOR:
            case GL_ONE_MINUS_CONSTANT_COLOR:
            case GL_CONSTANT_ALPHA:
            case GL_ONE_MINUS_CONSTANT_ALPHA:
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
            case GL_RGBA8:
            case GL_SRGB8_ALPHA8:
                return MetalCraftBridge.PIXEL_FORMAT_BGRA8_UNORM;
            case 0:
                return MetalCraftBridge.PIXEL_FORMAT_INVALID;
            default:
                return MetalCraftBridge.PIXEL_FORMAT_BGRA8_UNORM;
        }
    }

    private static boolean isTrackableTextureTarget(int target) {
        return target == GL_TEXTURE_2D || target == GL_TEXTURE_2D_MULTISAMPLE;
    }

    private static boolean isColorAttachment(int attachment) {
        return attachment >= GL_COLOR_ATTACHMENT0 && attachment < GL_COLOR_ATTACHMENT0 + 32;
    }

    private static final class ShaderState {
        int glShaderType;
        String source;
        boolean flipVertexY;
    }

    private static final class ProgramState {
        long vertexShaderId;
        long fragmentShaderId;
        long vertexLayoutId;
        boolean linked;
    }

    private static final class TextureState {
        int format = MetalCraftBridge.PIXEL_FORMAT_BGRA8_UNORM;
        int sampleCount = 1;
    }

    private static final class RenderbufferState {
        int format = MetalCraftBridge.PIXEL_FORMAT_BGRA8_UNORM;
        int sampleCount = 1;
    }

    private static final class FramebufferState {
        long colorTextureId;
        long colorRenderbufferId;
        long depthTextureId;
        long depthRenderbufferId;

        boolean hasDepthAttachment() {
            return depthTextureId != 0L || depthRenderbufferId != 0L;
        }
    }
}
