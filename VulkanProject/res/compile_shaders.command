DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"

if [ -n "${VK_SDK_PATH}" ]; then
    VULKAN_SDK_PATH="${VK_SDK_PATH}"
elif [ -n "${VULKAN_SDK}" ]; then
    VULKAN_SDK_PATH="${VULKAN_SDK}"
fi

if [ -n "${GLSLC_PATH}" ]; then
    GLSLC_EXE="${GLSLC_PATH}"
elif [ -n "${VULKAN_SDK_PATH}" ] && [ -x "${VULKAN_SDK_PATH}/Bin/glslc" ]; then
    GLSLC_EXE="${VULKAN_SDK_PATH}/Bin/glslc"
else
    GLSLC_EXE="/usr/local/bin/glslc"
fi

if [ -n "${DXC_PATH}" ]; then
    DXC_EXE="${DXC_PATH}"
elif [ -n "${VULKAN_SDK_PATH}" ] && [ -x "${VULKAN_SDK_PATH}/Bin/dxc" ]; then
    DXC_EXE="${VULKAN_SDK_PATH}/Bin/dxc"
else
    DXC_EXE="/usr/local/bin/dxc"
fi

if [ ! -x "${GLSLC_EXE}" ]; then
    echo "Error: glslc not found at '${GLSLC_EXE}'"
    exit 1
fi

USE_DXC_VALUE="${SHADER_USE_DXC:-1}"
echo "Shader compile mode: USE_DXC=${USE_DXC_VALUE}"

if [ "${USE_DXC_VALUE}" = "0" ]; then
    echo "Error: migrated shaders no longer have GLSL fallback. Set SHADER_USE_DXC=1."
    exit 1
fi

if [ ! -x "${DXC_EXE}" ]; then
    echo "Error: DXC not found at '${DXC_EXE}'"
    exit 1
fi

echo "Compiling migrated HLSL shaders with DXC..."
"${DXC_EXE}" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T vs_6_0 -E main shaders/vertex.hlsl        -Fo shaders/vertex.spv || exit 1
"${DXC_EXE}" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T vs_6_0 -E main shaders/aabb_debug_vs.hlsl -Fo shaders/aabb_debug_vs.spv || exit 1
"${DXC_EXE}" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T ps_6_0 -E main shaders/aabb_debug_fs.hlsl -Fo shaders/aabb_debug_fs.spv || exit 1
"${DXC_EXE}" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T vs_6_0 -E main shaders/fullscreenVS.hlsl  -Fo shaders/fullscreenVS.spv || exit 1
"${DXC_EXE}" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T ps_6_0 -E main shaders/fragment.hlsl      -Fo shaders/fragment.spv || exit 1
"${DXC_EXE}" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T ps_6_0 -E main shaders/tonemap.hlsl       -Fo shaders/tonemap.spv || exit 1
"${DXC_EXE}" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T cs_6_0 -E main shaders/cubemapgen.hlsl    -Fo shaders/cubemapgen.spv || exit 1
"${DXC_EXE}" -spirv -fspv-target-env=vulkan1.2 -fspv-extension=SPV_KHR_ray_tracing -T lib_6_6 -E main shaders/miss.hlsl -Fo shaders/miss.spv || exit 1

echo "Compiling remaining GLSL shaders with glslc..."
"${GLSLC_EXE}" -O -fhlsl-offsets --target-spv=spv1.5 --target-env=vulkan1.2 -fshader-stage=compute  shaders/raytracer.glsl     -o shaders/raytracer.spv || exit 1
"${GLSLC_EXE}" -O -fhlsl-offsets --target-spv=spv1.5 --target-env=vulkan1.2 -fshader-stage=rgen     shaders/raygen.glsl        -o shaders/raygen.spv || exit 1
"${GLSLC_EXE}" -O -fhlsl-offsets --target-spv=spv1.5 --target-env=vulkan1.2 -fshader-stage=rchit    shaders/closesthit.glsl    -o shaders/closesthit.spv || exit 1
"${GLSLC_EXE}" -O -fhlsl-offsets --target-spv=spv1.5 --target-env=vulkan1.2 -fshader-stage=rahit    shaders/anyhit.glsl        -o shaders/anyhit.spv || exit 1
