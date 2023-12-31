workspace "Vulkan-Project"
	architecture "x86_64"
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
		project "GLFW"
			kind("StaticLib")
			warnings("Off")
			intrinsics("On")
			editandcontinue("Off")
			language("C++")
			cppdialect("C++20")
			systemversion("latest")
			architecture("x86_64")
			exceptionhandling("Off")
			rtti("Off")
			floatingpoint("Fast")
			vectorextensions("SSE2")
			characterset("Ascii")
			flags(
			{ 
				"MultiProcessorCompile",
				"NoIncrementalLink",
			})

			location "Projectfiles/Dependencies/GLFW"
			
			-- Targets
			targetdir ("Build/bin/Dependencies/GLFW/" .. outputdir)
			objdir ("Build/bin-int/Dependencies/GLFW/" .. outputdir)

			-- All platforms
			files
			{
				"Dependencies/GLFW/include/GLFW/glfw3.h",
				"Dependencies/GLFW/include/GLFW/glfw3native.h",
				"Dependencies/GLFW/src/internal.h", 
				"Dependencies/GLFW/src/platform.h",
				"Dependencies/GLFW/src/mappings.h",
				"Dependencies/GLFW/src/context.c",
				"Dependencies/GLFW/src/init.c",
				"Dependencies/GLFW/src/input.c",
				"Dependencies/GLFW/src/monitor.c",
				"Dependencies/GLFW/src/platform.c",
				"Dependencies/GLFW/src/vulkan.c",
				"Dependencies/GLFW/src/window.c",
				"Dependencies/GLFW/src/egl_context.c",
				"Dependencies/GLFW/src/osmesa_context.c",
				"Dependencies/GLFW/src/null_platform.h",
				"Dependencies/GLFW/src/null_joystick.h",
				"Dependencies/GLFW/src/null_init.c",
				"Dependencies/GLFW/src/null_monitor.c",
				"Dependencies/GLFW/src/null_window.c",
				"Dependencies/GLFW/src/null_joystick.c"
			}
			
			-- macOS
			filter "system:macosx"
				systemversion "latest"
				staticruntime "On"

				files
				{
					"Dependencies/GLFW/src/cocoa_time.h",
					"Dependencies/GLFW/src/cocoa_time.c",
					"Dependencies/GLFW/src/posix_thread.h",
                    "Dependencies/GLFW/src/posix_module.c",
					"Dependencies/GLFW/src/posix_thread.c",
					"Dependencies/GLFW/src/cocoa_platform.h",
					"Dependencies/GLFW/src/cocoa_joystick.h",
					"Dependencies/GLFW/src/cocoa_init.m",
					"Dependencies/GLFW/src/cocoa_joystick.m",
					"Dependencies/GLFW/src/cocoa_monitor.m",
					"Dependencies/GLFW/src/cocoa_window.m",
					"Dependencies/GLFW/src/nsgl_context.m",
				}

				defines
				{
					"_GLFW_COCOA"
				}

			-- WIN32
			filter "system:windows"
				systemversion "latest"
				staticruntime "On"
				
				files
				{
					"Dependencies/GLFW/src/win32_time.h",
					"Dependencies/GLFW/src/win32_thread.h",
					"Dependencies/GLFW/src/win32_module.c",
					"Dependencies/GLFW/src/win32_time.c",
					"Dependencies/GLFW/src/win32_thread.c",
					"Dependencies/GLFW/src/win32_platform.h",
					"Dependencies/GLFW/src/win32_joystick.h",
					"Dependencies/GLFW/src/win32_init.c",
                    "Dependencies/GLFW/src/win32_joystick.c",
					"Dependencies/GLFW/src/win32_monitor.c",
					"Dependencies/GLFW/src/win32_window.c",
                    "Dependencies/GLFW/src/wgl_context.c"
				}

				defines 
				{ 
					"_GLFW_WIN32",
					"_CRT_SECURE_NO_WARNINGS"
				}

			-- Debug
			filter "configurations:Debug"
				runtime "Debug"
				symbols "on"

			-- Release
			filter "configurations:Release"
				runtime "Release"
				optimize "on"
			filter {}

		-- tinyobj Project
		project "tinyobj"
			kind("StaticLib")
			warnings("Off")
			intrinsics("On")
			editandcontinue("Off")
			language("C++")
			cppdialect("C++20")
			systemversion("latest")
			architecture("x86_64")
			exceptionhandling("Off")
			rtti("Off")
			floatingpoint("Fast")
			vectorextensions("SSE2")
			characterset("Ascii")
			flags(
			{ 
				"MultiProcessorCompile",
				"NoIncrementalLink",
			})

			location "Projectfiles/Dependencies/tinyobj"
			
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
			targetdir ("../Build/bin/Dependencies/tinyobj/" .. outputdir)
			objdir ("../Build/bin-int/Dependencies/tinyobj/" .. outputdir)
					
			-- Files
			files 
			{
				"Dependencies/tinyobj/tiny_obj_loader.h",
				"Dependencies/tinyobj/tiny_obj_loader.cc",
			}
	group ""
	
	-- Project
	project "VulkanProject"
		kind("ConsoleApp")
		staticruntime "on"
		warnings("Off")
		intrinsics("On")
		editandcontinue("Off")
		language("C++")
		cppdialect("C++20")
		systemversion("latest")
		architecture("x86_64")
		exceptionhandling("Off")
		rtti("Off")
		floatingpoint("Fast")
		vectorextensions("SSE2")
		characterset("Ascii")
		flags(
		{ 
			"MultiProcessorCompile",
			"NoIncrementalLink",
		})
		
		location "VulkanProject"

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