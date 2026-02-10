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

set GLSLC_PATH=%VULKAN_SDK_PATH%\Bin\glslc.exe
if not defined DXC_PATH (
    set DXC_PATH=%VULKAN_SDK_PATH%\Bin\dxc.exe
)

if not exist "%GLSLC_PATH%" (
    echo Error: glslc not found at "%GLSLC_PATH%".
    exit /b 1
)

set USE_DXC=1
if defined SHADER_USE_DXC (
    set USE_DXC=%SHADER_USE_DXC%
)

echo Shader compile mode: USE_DXC=%USE_DXC%

if "%USE_DXC%"=="0" (
    echo Error: migrated shaders no longer have GLSL fallback. Set SHADER_USE_DXC=1.
    exit /b 1
)

if not exist "%DXC_PATH%" (
    echo Error: DXC not found at "%DXC_PATH%".
    exit /b 1
)

echo Compiling migrated HLSL shaders with DXC...
"%DXC_PATH%" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T vs_6_0 -E main shaders/vertex.hlsl        -Fo shaders/vertex.spv
if errorlevel 1 exit /b 1
"%DXC_PATH%" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T vs_6_0 -E main shaders/aabb_debug_vs.hlsl -Fo shaders/aabb_debug_vs.spv
if errorlevel 1 exit /b 1
"%DXC_PATH%" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T ps_6_0 -E main shaders/aabb_debug_fs.hlsl -Fo shaders/aabb_debug_fs.spv
if errorlevel 1 exit /b 1
"%DXC_PATH%" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T vs_6_0 -E main shaders/fullscreenVS.hlsl  -Fo shaders/fullscreenVS.spv
if errorlevel 1 exit /b 1
"%DXC_PATH%" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T ps_6_0 -E main shaders/fragment.hlsl      -Fo shaders/fragment.spv
if errorlevel 1 exit /b 1
"%DXC_PATH%" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T ps_6_0 -E main shaders/tonemap.hlsl       -Fo shaders/tonemap.spv
if errorlevel 1 exit /b 1
"%DXC_PATH%" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T cs_6_0 -E main shaders/cubemapgen.hlsl    -Fo shaders/cubemapgen.spv
if errorlevel 1 exit /b 1
"%DXC_PATH%" -spirv -fspv-target-env=vulkan1.2 -fspv-extension=SPV_KHR_ray_tracing -T lib_6_6 -E main shaders/miss.hlsl -Fo shaders/miss.spv
if errorlevel 1 exit /b 1

echo Compiling remaining GLSL shaders with glslc...
"%GLSLC_PATH%" -O -fhlsl-offsets --target-spv=spv1.5 --target-env=vulkan1.2 -fshader-stage=compute  shaders/raytracer.glsl     -o shaders/raytracer.spv
if errorlevel 1 exit /b 1
"%GLSLC_PATH%" -O -fhlsl-offsets --target-spv=spv1.5 --target-env=vulkan1.2 -fshader-stage=rgen     shaders/raygen.glsl        -o shaders/raygen.spv
if errorlevel 1 exit /b 1
"%GLSLC_PATH%" -O -fhlsl-offsets --target-spv=spv1.5 --target-env=vulkan1.2 -fshader-stage=rchit    shaders/closesthit.glsl    -o shaders/closesthit.spv
if errorlevel 1 exit /b 1
"%GLSLC_PATH%" -O -fhlsl-offsets --target-spv=spv1.5 --target-env=vulkan1.2 -fshader-stage=rahit    shaders/anyhit.glsl        -o shaders/anyhit.spv
if errorlevel 1 exit /b 1

pause
