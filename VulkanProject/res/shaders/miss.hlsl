#include "common_structs.hlsli"
#define TRACE_COMMON_NO_TRACE_API
#include "trace_common.hlsli"

#define BACKGROUND_TYPE_NONE 0
#define BACKGROUND_TYPE_GRADIENT 1
#define BACKGROUND_TYPE_SKYBOX 2

struct SSceneBuffer
{
    SSceneSettings Settings;
};

[[vk::binding(16)]]
ConstantBuffer<SSceneBuffer> SceneBuffer : register(b16);

[[vk::combinedImageSampler]][[vk::binding(4)]]
TextureCube<float4> CubeTexture : register(t0);

[[vk::combinedImageSampler]][[vk::binding(4)]]
SamplerState CubeTextureSampler : register(s0);

[shader("miss")]
void main(inout SRayPayload RayPayload)
{
    uint  BackgroundType        = SceneBuffer.Settings.BackgroundType;
    float GradientLightStrength = SceneBuffer.Settings.GradientLightStrength;
    RayPayload.MissEmissive = GetEnvironmentLight(BackgroundType, GradientLightStrength, CubeTexture, CubeTextureSampler, WorldRayDirection(), 1.0);
}


