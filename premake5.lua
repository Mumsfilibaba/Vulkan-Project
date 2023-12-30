workspace "Vulkan-Project"
	architecture "x64"
	startproject "VulkanProject"
	warnings "Extra"
	
	-- Were to output files
	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

	-- Confingurations
	configurations 
	{
		"Debug", 
		"Release",
	}
	
	-- Platform
	platforms
	{
		"x64",
	}
	
	-- Debug builds
	filter "configurations:Debug"
		symbols "On"
		runtime "Debug"
		defines 
		{
			"DEBUG",
		}
	
	-- Release builds
	filter "configurations:Release"
		symbols "On"
		runtime "Release"
		optimize "Full"
		defines 
		{ 
			"NDEBUG",
		}
	filter {}
	
	-- Compiler option
	filter "action:vs*"
        defines
        {
            "COMPILER_VISUAL_STUDIO",
            "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING",
            "_CRT_SECURE_NO_WARNINGS",
        }

    -- OS
    filter "system:windows"
        defines
        {
            "PLATFORM_WINDOWS",
        }
    filter {}

	-- Dependencies
	group "Dependencies"
		include "Dependencies/GLFW"

		-- tinyobj Project
		project "tinyobj"
			kind "StaticLib"
			language "C++"
			cppdialect "C++20"
			systemversion "latest"
			staticruntime "on"
			location "Dependencies/projectfiles/tinyobj"
			
			filter "configurations:Debug"
				runtime "Debug"
			filter {}
			
			filter "configurations:Release"
				runtime "Release"
			filter {}

			filter "configurations:Debug or Release"
				symbols "on"
				optimize "Full"
			filter{}
			
			filter "configurations:Production"
				symbols "off"
				runtime "Release"
				optimize "Full"
			filter{}
			
			-- Targets
			targetdir ("Dependencies/bin/tinyobj/" .. outputdir)
			objdir ("Dependencies/bin-int/tinyobj/" .. outputdir)
					
			-- Files
			files 
			{
				"Dependencies/tinyobj/tiny_obj_loader.h",
				"Dependencies/tinyobj/tiny_obj_loader.cc",
			}
	group ""
	
	-- Project
	project "VulkanProject"
		language "C++"
		cppdialect "C++20"
		systemversion "latest"
		location "VulkanProject"
		staticruntime "on"
		kind "ConsoleApp"

		-- Targets
		builddir = "Build/bin/" .. outputdir .. "/%{prj.name}"
		targetdir 	(builddir)
		objdir 		("Build/bin-int/" .. outputdir .. "/%{prj.name}")	

		-- Files to include
		files 
		{ 
			"%{prj.name}/**.h",
			"%{prj.name}/**.hpp",
			"%{prj.name}/**.inl",
			"%{prj.name}/**.c",
			"%{prj.name}/**.cpp",
		}
		
		-- Windows
		filter "system:windows"
			links
			{
				"vulkan-1",
			}
			libdirs
			{
				"C:/VulkanSDK/1.2.148.1/Lib",
			}
			sysincludedirs
			{
				"C:/VulkanSDK/1.2.148.1/Include",
			}
			
			prebuildcommands
			{ 
				"compile_shaders" 
			}

			postbuildcommands 
			{ 
				"{COPY} res " .. builddir
			}

		-- macOS
		filter { "system:macosx" }
			links
			{
				"vulkan.1",
				"Cocoa.framework",
				"OpenGL.framework",
				"IOKit.framework",
				"CoreFoundation.framework"
			}
			libdirs
			{
				"/usr/local/lib",
			}
			sysincludedirs
			{
				"/usr/local/include",
			}

			prebuildcommands 
			{ 
				"./compile_shaders.command" 
			}
		
		-- Visual Studio
		filter { "action:vs*" }
			defines
			{
				"_CRT_SECURE_NO_WARNINGS"
			}
		filter {}
	
		-- Includes
		includedirs
		{
			"%{prj.name}/src",
		}
		sysincludedirs
		{
			"Dependencies/stb",
			"Dependencies/GLFW/include",
			"Dependencies/glm",
			"Dependencies/tinyobj",
		}
		
		-- Links
		links 
		{ 
			"GLFW",
			"tinyobj"
		}

		-- TODO: If the app actually needs to get signed, this needs to be revisited
		filter { "action:xcode4" }
			xcodebuildsettings 
			{
				["PRODUCT_BUNDLE_IDENTIFIER"] = "PathTracer",
				["CODE_SIGN_STYLE"]           = "Automatic",
				["ARCHS"]                     = "x86_64",               -- Specify the architecture(s) e.g., "x86_64" for Intel
				["ONLY_ACTIVE_ARCH"]          = "YES",                  -- We only want to build the current architecture
				["ENABLE_HARDENED_RUNTIME"]   = "NO",                   -- Hardened runtime is required for notarization
				["GENERATE_INFOPLIST_FILE"]   = "YES",                  -- Generate the .plist file for now
				-- ["CODE_SIGN_IDENTITY"]        = "Apple Development", -- Sets 'Signing Certificate' to 'Development'. Defaults to 'Sign to Run Locally'. Not doing this will crash your app if you upgrade the project when prompted by Xcode.
				
				-- Tell the executable where to find the frameworks. Path is relative to executable location inside .app bundle
				["LD_RUNPATH_SEARCH_PATHS"]   = "/usr/local/lib/ $(INSTALL_PATH) @executable_path/../Frameworks",
			}
		filter {}
	project "*"