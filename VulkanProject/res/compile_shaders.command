DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"

if [ -n "${VK_SDK_PATH}" ]; then
    VULKAN_SDK_PATH="${VK_SDK_PATH}"
elif [ -n "${VULKAN_SDK}" ]; then
    VULKAN_SDK_PATH="${VULKAN_SDK}"
fi

if [ -n "${DXC_PATH}" ]; then
    DXC_EXE="${DXC_PATH}"
elif [ -n "${VULKAN_SDK_PATH}" ] && [ -x "${VULKAN_SDK_PATH}/Bin/dxc" ]; then
    DXC_EXE="${VULKAN_SDK_PATH}/Bin/dxc"
else
    DXC_EXE="/usr/local/bin/dxc"
fi

USE_DXC_VALUE="${SHADER_USE_DXC:-1}"
echo "Shader compile mode: USE_DXC=${USE_DXC_VALUE}"
OUTPUT_DIR="shaders/compiled_shaders"

if [ "${USE_DXC_VALUE}" = "0" ]; then
    echo "Error: migrated shaders no longer have GLSL fallback. Set SHADER_USE_DXC=1."
    exit 1
fi

if [ ! -x "${DXC_EXE}" ]; then
    echo "Error: DXC not found at '${DXC_EXE}'"
    exit 1
fi

mkdir -p "${OUTPUT_DIR}" || exit 1

echo "Compiling migrated HLSL shaders with DXC..."
"${DXC_EXE}" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T vs_6_0 -E main shaders/vertex.hlsl        -Fo "${OUTPUT_DIR}/vertex.spv" || exit 1
"${DXC_EXE}" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T vs_6_0 -E main shaders/aabb_debug_vs.hlsl -Fo "${OUTPUT_DIR}/aabb_debug_vs.spv" || exit 1
"${DXC_EXE}" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T ps_6_0 -E main shaders/aabb_debug_fs.hlsl -Fo "${OUTPUT_DIR}/aabb_debug_fs.spv" || exit 1
"${DXC_EXE}" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T vs_6_0 -E main shaders/fullscreenVS.hlsl  -Fo "${OUTPUT_DIR}/fullscreenVS.spv" || exit 1
"${DXC_EXE}" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T ps_6_0 -E main shaders/fragment.hlsl      -Fo "${OUTPUT_DIR}/fragment.spv" || exit 1
"${DXC_EXE}" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T ps_6_0 -E main shaders/tonemap.hlsl       -Fo "${OUTPUT_DIR}/tonemap.spv" || exit 1
"${DXC_EXE}" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -T cs_6_0 -E main shaders/cubemapgen.hlsl    -Fo "${OUTPUT_DIR}/cubemapgen.spv" || exit 1
"${DXC_EXE}" -spirv -fspv-target-env=vulkan1.2 -fspv-extension=SPV_KHR_ray_tracing -T lib_6_6 -E main shaders/miss.hlsl -Fo "${OUTPUT_DIR}/miss.spv" || exit 1

echo "Compiling ray tracing shaders with DXC..."
"${DXC_EXE}" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -fspv-extension=SPV_EXT_descriptor_indexing -T cs_6_0 -E main shaders/raytracer.hlsl -Fo "${OUTPUT_DIR}/raytracer.spv" || exit 1
"${DXC_EXE}" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -fspv-extension=SPV_KHR_ray_tracing -fspv-extension=SPV_EXT_descriptor_indexing -T lib_6_6 -E main shaders/raygen.hlsl -Fo "${OUTPUT_DIR}/raygen.spv" || exit 1
"${DXC_EXE}" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -fspv-extension=SPV_KHR_ray_tracing -fspv-extension=SPV_KHR_physical_storage_buffer -T lib_6_6 -E main shaders/closesthit.hlsl -Fo "${OUTPUT_DIR}/closesthit.spv" || exit 1
"${DXC_EXE}" -spirv -fspv-target-env=vulkan1.2 -fvk-use-dx-layout -fspv-extension=SPV_KHR_ray_tracing -fspv-extension=SPV_KHR_physical_storage_buffer -fspv-extension=SPV_EXT_descriptor_indexing -T lib_6_6 -E main shaders/anyhit.hlsl -Fo "${OUTPUT_DIR}/anyhit.spv" || exit 1
