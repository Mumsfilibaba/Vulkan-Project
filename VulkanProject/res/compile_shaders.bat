@echo off
setlocal EnableDelayedExpansion

cd %~dp0

if not defined VK_SDK_PATH (
    if not defined VULKAN_SDK (
        echo Error: Vulkan SDK environment variable not set.
        exit /b 1
    ) else (
        set VULKAN_SDK_PATH=%VULKAN_SDK%
    )
) else (
    set VULKAN_SDK_PATH=%VK_SDK_PATH%
)

if not defined DXC_PATH (
    set DXC_PATH=%VULKAN_SDK_PATH%\Bin\dxc.exe
)

set OUTPUT_DIR=shaders/compiled_shaders

if not exist "%DXC_PATH%" (
    echo Error: DXC not found at "%DXC_PATH%".
    exit /b 1
)

if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
    if errorlevel 1 exit /b 1
)

echo Compiling HLSL shaders with DXC...
"%DXC_PATH%" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T vs_6_0 -E main shaders/vertex.hlsl        -Fo %OUTPUT_DIR%/vertex.spv
if errorlevel 1 exit /b 1
"%DXC_PATH%" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T vs_6_0 -E main shaders/aabb_debug_vs.hlsl -Fo %OUTPUT_DIR%/aabb_debug_vs.spv
if errorlevel 1 exit /b 1
"%DXC_PATH%" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T ps_6_0 -E main shaders/aabb_debug_fs.hlsl -Fo %OUTPUT_DIR%/aabb_debug_fs.spv
if errorlevel 1 exit /b 1
"%DXC_PATH%" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T vs_6_0 -E main shaders/fullscreenVS.hlsl  -Fo %OUTPUT_DIR%/fullscreenVS.spv
if errorlevel 1 exit /b 1
"%DXC_PATH%" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T ps_6_0 -E main shaders/fragment.hlsl      -Fo %OUTPUT_DIR%/fragment.spv
if errorlevel 1 exit /b 1
"%DXC_PATH%" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T ps_6_0 -E main shaders/tonemap.hlsl       -Fo %OUTPUT_DIR%/tonemap.spv
if errorlevel 1 exit /b 1
"%DXC_PATH%" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T cs_6_0 -E main shaders/cubemapgen.hlsl    -Fo %OUTPUT_DIR%/cubemapgen.spv
if errorlevel 1 exit /b 1
"%DXC_PATH%" -spirv -fspv-target-env=vulkan1.2 -fspv-extension=SPV_KHR_ray_tracing -T lib_6_6 -E main shaders/miss.hlsl -Fo %OUTPUT_DIR%/miss.spv
if errorlevel 1 exit /b 1

echo Compiling ray tracing shaders with DXC...
"%DXC_PATH%" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -fspv-extension=SPV_EXT_descriptor_indexing -T cs_6_0 -E main shaders/raytracer.hlsl -Fo %OUTPUT_DIR%/raytracer.spv
if errorlevel 1 exit /b 1
"%DXC_PATH%" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -fspv-extension=SPV_KHR_ray_tracing -fspv-extension=SPV_EXT_descriptor_indexing -T lib_6_6 -E main shaders/raygen.hlsl -Fo %OUTPUT_DIR%/raygen.spv
if errorlevel 1 exit /b 1
"%DXC_PATH%" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -fspv-extension=SPV_KHR_ray_tracing -fspv-extension=SPV_KHR_physical_storage_buffer -T lib_6_6 -E main shaders/closesthit.hlsl -Fo %OUTPUT_DIR%/closesthit.spv
if errorlevel 1 exit /b 1
"%DXC_PATH%" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -fspv-extension=SPV_KHR_ray_tracing -fspv-extension=SPV_KHR_physical_storage_buffer -fspv-extension=SPV_EXT_descriptor_indexing -T lib_6_6 -E main shaders/anyhit.hlsl -Fo %OUTPUT_DIR%/anyhit.spv
if errorlevel 1 exit /b 1

pause
