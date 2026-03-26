package net.kdt.pojavlaunch;

import java.beans.Beans;
import java.io.*;
import java.lang.reflect.Field;
import java.util.*;
import java.util.concurrent.*;

import org.lwjgl.glfw.CallbackBridge;
import org.lwjgl.glfw.GLFW;

import net.kdt.pojavlaunch.render.MetalCraftBridge;
import net.kdt.pojavlaunch.render.MetalCraftGLInterceptor;
import net.kdt.pojavlaunch.uikit.*;
import net.kdt.pojavlaunch.utils.*;
import net.kdt.pojavlaunch.value.*;

public class PojavLauncher {
    private static float currProgress, maxProgress;

    public static void main(String[] args) throws Throwable {
        // Skip calling to com.apple.eawt.Application.nativeInitializeApplicationDelegate()
        Beans.setDesignTime(true);
        try {
            // Some places use macOS-specific code, which is unavailable on iOS
            // In this case, try to get it to use Linux-specific code instead.
            Class<?> clazz = Class.forName("com.apple.eawt.Application");
            clazz.getMethod("getApplication").invoke(null);
            Field field = clazz.getDeclaredField("sApplication");
            field.setAccessible(true);
            field.set(null, null);
            try {
                Class<?> fontUtilities = Class.forName("sun.font.FontUtilities");
                Field isLinux = fontUtilities.getDeclaredField("isLinux");
                isLinux.setAccessible(true);
                isLinux.setBoolean(null, true);
            } catch (Throwable ignored) {}
            System.setProperty("java.util.prefs.PreferencesFactory", "java.util.prefs.FileSystemPreferencesFactory");
        } catch (Throwable th) {
            // Not on JRE8, ignore exception
            //Tools.showError(th);
        }

        Thread.currentThread().setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {

            public void uncaughtException(Thread t, Throwable th) {
                th.printStackTrace();
                System.exit(1);
            }
        });

        try {
            // Try to initialize Caciocavallo17
            Class.forName("com.github.caciocavallosilano.cacio.ctc.CTCPreloadClassLoader");
        } catch (ClassNotFoundException e) {}

        if (args[0].equals("-jar")) {
            UIKit.callback_JavaGUIViewController_launchJarFile(args[1], Arrays.copyOfRange(args, 2, args.length));
        } else {
            launchMinecraft(args);
        }
    }

    public static void launchMinecraft(String[] args) throws Throwable {
        // Args for Spiral Knights
        System.setProperty("appdir", "./spiral");
        System.setProperty("resource_dir", "./spiral/rsrc");

        String sizeStr = System.getProperty("cacio.managed.screensize");
        System.setProperty("glfw.windowSize", sizeStr);
        String[] size = sizeStr.split("x");
        MCOptionUtils.load();
        MCOptionUtils.set("fullscreen", "false");
        MCOptionUtils.set("overrideWidth", size[0]);
        MCOptionUtils.set("overrideHeight", size[1]);
        // Default settings for performance
        MCOptionUtils.setDefault("mipmapLevels", "0");
        MCOptionUtils.setDefault("particles", "1");
        MCOptionUtils.setDefault("renderDistance", "2");
        MCOptionUtils.setDefault("simulationDistance", "5");
        MCOptionUtils.save();

        // Setup Forge splash.properties
        File forgeSplashFile = new File(Tools.DIR_GAME_NEW, "config/splash.properties");
        if (System.getProperty("pojav.internal.keepForgeSplash") == null) {
            forgeSplashFile.getParentFile().mkdir();
            if (forgeSplashFile.exists()) {
                Tools.write(forgeSplashFile.getAbsolutePath(), Tools.read(forgeSplashFile.getAbsolutePath().replace("enabled=true", "enabled=false")));
            } else {
                Tools.write(forgeSplashFile.getAbsolutePath(), "enabled=false");
            }
        }

        // Bypass Sodium's PojavLauncher/Amethyst detection to prevent crash
        // Sodium scans stack traces for "net.kdt.pojavlaunch" and checks these properties
        System.setProperty("sodium.checks.issue2561", "false");
        System.setProperty("sodium.checks.environment", "false");

        System.setProperty("org.lwjgl.vulkan.libname", "libMoltenVK.dylib");
        MetalCraftBridge.bootstrapRequestedRenderer();
        System.out.println("[MetalCraft] Bootstrap: available=" + MetalCraftBridge.isAvailable()
                + " metalcraft.active=" + System.getProperty("pojav.renderer.metalcraft.active")
                + " metalcraft=" + System.getProperty("pojav.renderer.metalcraft"));
        boolean interceptorActive = MetalCraftGLInterceptor.bootstrap();
        System.out.println("[MetalCraft] Interceptor bootstrap result: " + interceptorActive
                + " (isActive=" + MetalCraftGLInterceptor.isActive() + ")");

        MinecraftAccount account = MinecraftAccount.load(args[0]);
        JMinecraftVersionList.Version version = Tools.getVersionInfo(args[1]);
        System.out.println("Launching Minecraft " + version.id);
        String configPath;
        if (version.logging != null && version.logging.client != null && version.logging.client.file != null
                && version.logging.client.file.id != null) {
            if (version.logging.client.file.id.equals("client-1.12.xml")) {
                configPath = Tools.DIR_BUNDLE + "/log4j-rce-patch-1.12.xml";
            } else if (version.logging.client.file.id.equals("client-1.7.xml")) {
                configPath = Tools.DIR_BUNDLE + "/log4j-rce-patch-1.7.xml";
            } else {
                configPath = Tools.DIR_GAME_NEW + "/" + version.logging.client.file.id;
            }
            File configFile = new File(configPath);
            if (configFile.isFile()) {
                System.setProperty("log4j.configurationFile", configFile.getAbsolutePath());
            } else {
                System.out.println("[Log4j] Logging configuration missing, using default: " + configPath);
            }
        }

        Tools.launchMinecraft(account, version);
    }
}
