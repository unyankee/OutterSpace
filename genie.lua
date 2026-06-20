
-- Get absolute paths
local projectRoot = path.getabsolute(".")
local engineDir = path.getabsolute("./Engine")

solution "ToyEngine"
    location "build"
    configurations { "Debug", "Release" }
    platforms { "x64" }

project "Engine"
    kind "ConsoleApp"
    language "C++"
    location "build"
    
    flags {
        "Cpp20",
        "Unicode",
        "NoIncrementalLink",
    }

    includedirs {
        ".",
        "Engine",
        "extern/meshoptimizer/src",
        "extern/volk",
        "extern/glfw/glfw-3.4/include",
        "extern/imgui",
        "extern/entt/src",
        "extern/glm",
        "$(VULKAN_SDK)/include"
    }

    defines {
        "WIN32_LEAN_AND_MEAN",
        "NOMINMAX",
        "_GLFW_WIN32",
        "GLFW_EXPOSE_NATIVE_WIN32",
        "VK_USE_PLATFORM_WIN32_KHR",
        "_CRT_SECURE_NO_WARNINGS",
        -- Global path defines as raw string literals
        "ENGINE_PROJECT_ROOT=R\"(" .. projectRoot .. ")\"",
        "ENGINE_DIR=R\"(" .. engineDir .. ")\""
    }

    files {
        "Engine/main.cpp",
        "Engine/src/*.cpp",
        "Engine/src/*.h",
        "Engine/Common/**.h", -- Add Common directory files
        "extern/volk/volk.c",
        "extern/volk/volk.h",
        "extern/meshoptimizer/src/**.cpp",
        "extern/meshoptimizer/src/**.h",
        "extern/meshoptimizer/tools/objloader.cpp",
        -- ImGui Source Files
        "extern/imgui/**.cpp",
        -- GLFW Source Files (Windows specific)
        "extern/glfw/glfw-3.4/src/context.c",
        "extern/glfw/glfw-3.4/src/egl_context.c",
        "extern/glfw/glfw-3.4/src/init.c",
        "extern/glfw/glfw-3.4/src/input.c",
        "extern/glfw/glfw-3.4/src/monitor.c",
        "extern/glfw/glfw-3.4/src/null_init.c",
        "extern/glfw/glfw-3.4/src/null_joystick.c",
        "extern/glfw/glfw-3.4/src/null_monitor.c",
        "extern/glfw/glfw-3.4/src/null_window.c",
        "extern/glfw/glfw-3.4/src/osmesa_context.c",
        "extern/glfw/glfw-3.4/src/platform.c",
        "extern/glfw/glfw-3.4/src/vulkan.c",
        "extern/glfw/glfw-3.4/src/wgl_context.c",
        "extern/glfw/glfw-3.4/src/win32_init.c",
        "extern/glfw/glfw-3.4/src/win32_joystick.c",
        "extern/glfw/glfw-3.4/src/win32_module.c",
        "extern/glfw/glfw-3.4/src/win32_monitor.c",
        "extern/glfw/glfw-3.4/src/win32_thread.c",
        "extern/glfw/glfw-3.4/src/win32_time.c",
        "extern/glfw/glfw-3.4/src/win32_window.c",
        "extern/glfw/glfw-3.4/src/window.c",
        --- GLM
        "extern/glm/glm/**.hpp",
        "extern/glm/glm/**.h",
        "extern/glm/glm/**.inl",
    }

    vpaths {
        ["Source/*"] = { "Engine/**" },
        ["Extern/volk"] = { "extern/volk/**" },
        ["Extern/meshoptimizer"] = { "extern/meshoptimizer/**" },
        ["Extern/glfw"] = { "extern/glfw/**" },
        ["Extern/imgui"] = { "extern/imgui/**" },
        ["Extern/glm"] = { "extern/glm/**" },
    }

    links {
        "vulkan-1"
    }

    libdirs {
        "$(VULKAN_SDK)/lib"
    }

    configuration "Debug"
        flags { "Symbols" }
        defines { "_DEBUG" }
        targetdir "bin/Debug"

    configuration "Release"
        flags { "OptimizeSpeed" }
        targetdir "bin/Release"

    -- Fix the entry point for ConsoleApp on Windows when using main()
    configuration "windows"
        linkoptions { "/ENTRY:mainCRTStartup" }
