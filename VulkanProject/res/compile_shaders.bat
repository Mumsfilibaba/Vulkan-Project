@echo off

:: Set the working directory to the location of the script
cd %~dp0

:: Check if VK_SDK_PATH is set
if not defined VK_SDK_PATH (
    :: If VK_SDK_PATH is not set, check if VULKAN_SDK is set
    if not defined VULKAN_SDK (
        echo Error: Vulkan SDK environment variable not set.
        :: pause
        exit /b 1
    ) else (
        set VULKAN_SDK_PATH=%VULKAN_SDK%
    )
) else (
    set VULKAN_SDK_PATH=%VK_SDK_PATH%
)

:: Build paths to glslc.exe and shader files using environment variables
set GLSLC_PATH=%VULKAN_SDK_PATH%\Bin\glslc.exe

%GLSLC_PATH% -fshader-stage=vertex   shaders/vertex.glsl     -o shaders/vertex.spv
%GLSLC_PATH% -fshader-stage=fragment shaders/fragment.glsl   -o shaders/fragment.spv
%GLSLC_PATH% -fshader-stage=compute  shaders/raytracer.glsl  -o shaders/raytracer.spv
%GLSLC_PATH% -fshader-stage=compute  shaders/cubemapgen.glsl -o shaders/cubemapgen.spv
:: pause