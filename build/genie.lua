solution "Engine_Dev"
-- set the global project directory 
PROJ_DIR = path.getabsolute("..")

-- .sln project should be generated in this directory
location (path.join(PROJ_DIR, "build"))

project "Vulkan_renderer"
	
	configuration "vs2019"
	
	-- Need to find a way to get the latest windowsSDK installed !!
	windowstargetplatformversion "10.0.17763.0"

	kind "ConsoleApp"
	
	location (path.join(PROJ_DIR, "build/Vulkan_renderer"))
	targetdir (path.join(PROJ_DIR, "output"))

	debugdir (path.join(PROJ_DIR, "debug"))

	-- collection of needed Defines 
	links
	{ 
		(path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/lib/vulkan-1")),		
		"kernel32",
		"user32",
		"gdi32",
		"winspool",
		"shell32",
		"ole32",
		"oleaut32",
		"uuid",
		"comdlg32",
		"advapi32",
	}

	defines 
	{ 
		"WIN32",
		"VK_USE_64_BIT_PTR_DEFINES=0",
		"_WINDOWS",
		"VK_USE_PLATFORM_WIN32_KHR",
		"NOMINMAX",
		"_USE_MATH_DEFINES",
		"_CRT_SECURE_NO_WARNINGS",  
		"GLM_FORCE_DEPTH_ZERO_TO_ONE",
		"GLM_FORCE_XYZW_ONLY",
		-- "GLM_FORCE_LEFT_HANDED",
	}

	configurations 
	{
		"Debug", 
		"Release",
	}

	platforms
	{
		"x64",
	}

	includedirs 
	{
		-- Engine includes
		path.join(PROJ_DIR, "engine/include"),
		-- GLM includes
		path.join(PROJ_DIR, "extern/glm-0.9.9.8/glm/"),
		-- VULKAN SDK 
		path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1"),
		-- Root for extern code
		path.join(PROJ_DIR, "extern/"),
		-- json parsing
		path.join(PROJ_DIR, "extern/simdjson/"),
		-- imgui
		path.join(PROJ_DIR, "extern/imgui/"),		
		-- shaderc
		path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/Include/shaderc"),		
		path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/Include/"),		
	}
	
	files
	{
		-- Engine srd and include files
		path.join(PROJ_DIR, "engine/include/*.h"),
		path.join(PROJ_DIR, "engine/src/*.cpp"),
		-- json parsing
		 path.join(PROJ_DIR, "extern/simdjson/simdjson.cpp"),	
		 -- imgui
		 path.join(PROJ_DIR, "extern/imgui/*.cpp"),	
		 -- spv cross
		 path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/Include/spirv_cross/*.hpp"),	
		 path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/Include/spirv_cross/*.cpp"),	
	}
	
	configuration "Debug"
		defines { "DEBUG" }
		flags { "Symbols", "ExtraWarnings","Cpp17"}
  		links 
  		{ 
  			(path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/lib/shaderc_combinedd")),
  			-- (path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/lib/spirv-cross-c")),
  			(path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/lib/spirv-cross-cored")),
  			(path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/lib/spirv-cross-cppd")),
  			(path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/lib/spirv-cross-c-sharedd")),
  			(path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/lib/spirv-cross-glsld")),
  			(path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/lib/spirv-cross-hlsld")),
  			(path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/lib/spirv-cross-msld")),
  			(path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/lib/spirv-cross-reflectd")),
  			(path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/lib/spirv-cross-utild")),  			
  		}

	configuration "Release"
		defines { "NDEBUG" }
		flags { "Optimize", "ExtraWarnings","Cpp17"}
		links 
  		{ 
  			(path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/lib/shaderc_combined")),
  			-- (path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/lib/spirv-cross-c")),
  			(path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/lib/spirv-cross-core")),
  			(path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/lib/spirv-cross-cpp")),
  			(path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/lib/spirv-cross-c-shared")),
  			(path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/lib/spirv-cross-glsl")),
  			(path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/lib/spirv-cross-hlsl")),
  			(path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/lib/spirv-cross-msl")),
  			(path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/lib/spirv-cross-reflect")),
  			(path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/lib/spirv-cross-util")),  			
  		}


	language "C++" 


project "Vulkan_glsl_compiler_and_reflection"
	
	configuration "vs2019"
	language "C++" 
	kind "ConsoleApp"
	
	-- Need to find a way to get the latest windowsSDK installed !!
	windowstargetplatformversion "10.0.17763.0"

	postbuildcommands { path.join(PROJ_DIR, "engine/data/shaders/glsl/compileshaders.py") }

	location (path.join(PROJ_DIR, "build/Vulkan_renderer/GLSL_compiler_reflection"))
	targetdir (path.join(PROJ_DIR, "tools/shader_reflection/output"))

	debugdir (path.join(PROJ_DIR, "debug"))

	includedirs 
	{
		path.join(PROJ_DIR, "tools/shader_reflection/include/"),
		path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/"),
		path.join(PROJ_DIR, "extern/simdjson/"),
		path.join(PROJ_DIR, "extern/json/nlohmann"),
		path.join(PROJ_DIR, "extern/"),
	}

	files
	{
		-- spir v reflection, need to revisit this, since is quite messy atm
		 path.join(PROJ_DIR, "extern/VulkanSDK/1.2.198.1/Source/SPIRV-Reflect/spirv_reflect.c"),	
		-- actual shader reflection code
		 path.join(PROJ_DIR, "tools/shader_reflection/include/*.h"),	
		 path.join(PROJ_DIR, "tools/shader_reflection/src/*.cpp"),	
		 
	}

	configurations 
	{
		"Debug", 
		"Release",
	}


-- collection of needed Defines 
	links
	{ 
		"kernel32",
		"user32",
		"gdi32",
		"winspool",
		"shell32",
		"ole32",
		"oleaut32",
		"uuid",
		"comdlg32",
		"advapi32",
	}

	
	configuration "Debug"
		defines { "DEBUG" }
		flags { "Symbols", "ExtraWarnings", "Cpp17"}
  
	configuration "Release"
		defines { "NDEBUG" }
		flags { "Optimize", "ExtraWarnings", "Cpp17"}



project "Vulkan_glsl_compiler"
	
	configuration "vs2019"
	language "C++" 
	kind "ConsoleApp"
	
	-- Need to find a way to get the latest windowsSDK installed !!
	windowstargetplatformversion "10.0.17763.0"
	postbuildcommands { path.join(PROJ_DIR, "engine/data/shaders/glsl/compileshaders.py") }
	location (path.join(PROJ_DIR, "build/Vulkan_renderer/GLSL_compiler"))
	targetdir (path.join(PROJ_DIR, "tools/shader_compilation"))
