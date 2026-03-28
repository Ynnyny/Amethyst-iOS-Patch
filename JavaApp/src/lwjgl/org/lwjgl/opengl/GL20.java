package org.lwjgl.opengl;

import java.nio.ByteBuffer;
import java.nio.IntBuffer;
import java.nio.charset.StandardCharsets;

import net.kdt.pojavlaunch.render.MetalCraftGLInterceptor;

import org.lwjgl.PointerBuffer;
import org.lwjgl.system.MemoryStack;
import org.lwjgl.system.MemoryUtil;

public class GL20 extends GL20C {
    private static final String METALCRAFT_LINK_FAILURE =
            "MetalCraft program link failed: missing translated vertex or fragment shader";

    private GL20() {}

    public static int glCreateProgram() {
        return GL20C.glCreateProgram();
    }

    public static void glDeleteProgram(int program) {
        MetalCraftGLInterceptor.onProgramDeleted(MetalCraftOpenGLHooks.asUnsignedId(program));
        GL20C.glDeleteProgram(program);
    }

    public static int glCreateShader(int type) {
        int shader = GL20C.glCreateShader(type);
        MetalCraftGLInterceptor.onShaderCreated(MetalCraftOpenGLHooks.asUnsignedId(shader), type);
        return shader;
    }

    public static void glDeleteShader(int shader) {
        MetalCraftGLInterceptor.onShaderDeleted(MetalCraftOpenGLHooks.asUnsignedId(shader));
        GL20C.glDeleteShader(shader);
    }

    public static void glAttachShader(int program, int shader) {
        if (!MetalCraftGLInterceptor.isActive()) {
            GL20C.glAttachShader(program, shader);
        }
        MetalCraftGLInterceptor.onShaderAttached(MetalCraftOpenGLHooks.asUnsignedId(program),
                MetalCraftOpenGLHooks.asUnsignedId(shader));
    }

    public static void glDetachShader(int program, int shader) {
        if (!MetalCraftGLInterceptor.isActive()) {
            GL20C.glDetachShader(program, shader);
        }
        MetalCraftGLInterceptor.onShaderDetached(MetalCraftOpenGLHooks.asUnsignedId(program),
                MetalCraftOpenGLHooks.asUnsignedId(shader));
    }

    public static void glShaderSource(int shader, ByteBuffer string) {
        if (string == null) {
            throw new NullPointerException("string");
        }

        ByteBuffer copy = string.duplicate();
        byte[] bytes = new byte[copy.remaining()];
        copy.get(bytes);
        MetalCraftGLInterceptor.registerShaderSource(
                MetalCraftOpenGLHooks.asUnsignedId(shader),
                new String(bytes, StandardCharsets.UTF_8), true);

        MemoryStack stack = MemoryStack.stackGet();
        int stackPointer = stack.getPointer();
        try {
            PointerBuffer strings = stack.mallocPointer(1);
            strings.put(0, MemoryUtil.memAddress(string));
            IntBuffer lengths = stack.ints(string.remaining());
            GL20C.glShaderSource(shader, strings, lengths);
        } finally {
            stack.setPointer(stackPointer);
        }
    }

    public static void glShaderSource(int shader, CharSequence... strings) {
        MetalCraftGLInterceptor.registerShaderSource(
                MetalCraftOpenGLHooks.asUnsignedId(shader),
                MetalCraftOpenGLHooks.concat(strings), true);
        GL20C.glShaderSource(shader, strings);
    }

    public static void glShaderSource(int shader, CharSequence string) {
        MetalCraftGLInterceptor.registerShaderSource(
                MetalCraftOpenGLHooks.asUnsignedId(shader),
                string == null ? null : string.toString(), true);
        GL20C.glShaderSource(shader, string);
    }

    public static void glCompileShader(int shader) {
        if (MetalCraftGLInterceptor.isActive()) {
            return;
        }
        GL20C.glCompileShader(shader);
    }

    public static void glLinkProgram(int program) {
        if (MetalCraftGLInterceptor.isActive()) {
            long programId = MetalCraftOpenGLHooks.asUnsignedId(program);
            if (MetalCraftGLInterceptor.canLinkProgram(programId)) {
                MetalCraftGLInterceptor.onProgramLinked(programId);
            } else {
                MetalCraftGLInterceptor.onProgramLinkFailed(programId);
            }
            return;
        }

        GL20C.glLinkProgram(program);
        if (GL20C.glGetProgrami(program, GL20C.GL_LINK_STATUS) == GL11C.GL_TRUE) {
            MetalCraftGLInterceptor.onProgramLinked(MetalCraftOpenGLHooks.asUnsignedId(program));
        } else {
            MetalCraftGLInterceptor.onProgramLinkFailed(
                    MetalCraftOpenGLHooks.asUnsignedId(program));
        }
    }

    public static int glGetShaderi(int shader, int pname) {
        if (MetalCraftGLInterceptor.isActive()) {
            if (pname == GL20C.GL_COMPILE_STATUS) {
                return MetalCraftGLInterceptor.hasShaderSource(
                        MetalCraftOpenGLHooks.asUnsignedId(shader)) ? GL11C.GL_TRUE
                                : GL11C.GL_FALSE;
            }
            if (pname == GL20C.GL_INFO_LOG_LENGTH) {
                return 0;
            }
            if (pname == GL20C.GL_SHADER_TYPE) {
                int shaderType = MetalCraftGLInterceptor.getShaderType(
                        MetalCraftOpenGLHooks.asUnsignedId(shader));
                if (shaderType != 0) {
                    return shaderType;
                }
            }
        }
        return GL20C.glGetShaderi(shader, pname);
    }

    public static String glGetShaderInfoLog(int shader) {
        if (MetalCraftGLInterceptor.isActive()) {
            return "";
        }
        return GL20C.glGetShaderInfoLog(shader);
    }

    public static String glGetShaderInfoLog(int shader, int maxLength) {
        if (MetalCraftGLInterceptor.isActive()) {
            return "";
        }
        return GL20C.glGetShaderInfoLog(shader, maxLength);
    }

    public static int glGetProgrami(int program, int pname) {
        if (MetalCraftGLInterceptor.isActive()) {
            if (pname == GL20C.GL_LINK_STATUS || pname == GL20C.GL_VALIDATE_STATUS) {
                return MetalCraftGLInterceptor.isProgramLinked(
                        MetalCraftOpenGLHooks.asUnsignedId(program)) ? GL11C.GL_TRUE
                                : GL11C.GL_FALSE;
            }
            if (pname == GL20C.GL_INFO_LOG_LENGTH) {
                return MetalCraftGLInterceptor.isProgramLinked(
                        MetalCraftOpenGLHooks.asUnsignedId(program)) ? 0
                                : METALCRAFT_LINK_FAILURE.length();
            }
        }
        return GL20C.glGetProgrami(program, pname);
    }

    public static String glGetProgramInfoLog(int program) {
        if (MetalCraftGLInterceptor.isActive()) {
            return MetalCraftGLInterceptor.isProgramLinked(
                    MetalCraftOpenGLHooks.asUnsignedId(program)) ? ""
                            : METALCRAFT_LINK_FAILURE;
        }
        return GL20C.glGetProgramInfoLog(program);
    }

    public static String glGetProgramInfoLog(int program, int maxLength) {
        if (MetalCraftGLInterceptor.isActive()) {
            String log = MetalCraftGLInterceptor.isProgramLinked(
                    MetalCraftOpenGLHooks.asUnsignedId(program)) ? ""
                            : METALCRAFT_LINK_FAILURE;
            if (maxLength <= 0 || log.length() <= maxLength) {
                return log;
            }
            return log.substring(0, maxLength);
        }
        return GL20C.glGetProgramInfoLog(program, maxLength);
    }

    public static void glValidateProgram(int program) {
        if (MetalCraftGLInterceptor.isActive()) {
            return;
        }
        GL20C.glValidateProgram(program);
    }

    public static void glUseProgram(int program) {
        if (!MetalCraftGLInterceptor.isActive()) {
            GL20C.glUseProgram(program);
        }
        MetalCraftGLInterceptor.useProgram(MetalCraftOpenGLHooks.asUnsignedId(program));
    }

    public static void glBlendEquationSeparate(int modeRGB, int modeAlpha) {
        MetalCraftGLInterceptor.setBlendEquationSeparate(modeRGB, modeAlpha);
        GL20C.glBlendEquationSeparate(modeRGB, modeAlpha);
    }
}
