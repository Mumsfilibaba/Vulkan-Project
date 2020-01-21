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
	
	-- Project
	project "VulkanProject"
		language "C++"
		cppdialect "C++17"
		systemversion "latest"
		location "VulkanProject"
		kind "ConsoleApp"

		-- Targets
		targetdir 	("Build/bin/" .. outputdir .. "/%{prj.name}")
		objdir 		("Build/bin-int/" .. outputdir .. "/%{prj.name}")	
		
		-- Files to include
		files 
		{ 
			"%{prj.name}/**.h",
			"%{prj.name}/**.hpp",
			"%{prj.name}/**.inl",
			"%{prj.name}/**.c",
			"%{prj.name}/**.cpp",
			"%{prj.name}/**.hlsl",
		}
		
		-- Windows
		filter "system:windows"
			links
			{
				"vulkan-1",
			}
			libdirs
			{
				"C:/VulkanSDK/1.1.121.2/Lib",
			}
			sysincludedirs
			{
				"C:/VulkanSDK/1.1.121.2/Include",
			}
		filter {}
		
		-- Includes
		includedirs
		{
			"%{prj.name}/Include",
		}
		sysincludedirs
		{
			"Dependencies/stb",
			"Dependencies/glm",
		}
	project "*"