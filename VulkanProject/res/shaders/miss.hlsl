#include "hw_ray_trace_common.hlsli"

#define BACKGROUND_TYPE_NONE 0
#define BACKGROUND_TYPE_GRADIENT 1
#define BACKGROUND_TYPE_SKYBOX 2

struct SSceneBuffer
{
    SSceneSettings settings;
};

[[vk::binding(5, 0)]]
ConstantBuffer<SSceneBuffer> sceneBuffer : register(b5, space0);

[[vk::combinedImageSampler]][[vk::binding(0, 1)]]
TextureCube<float4> cubeTexture : register(t0, space1);

[[vk::combinedImageSampler]][[vk::binding(0, 1)]]
SamplerState cubeTextureSampler : register(s0, space1);

[shader("miss")]
void main(inout SRayPayLoad rayPayload)
{
    if (sceneBuffer.settings.BackgroundType == BACKGROUND_TYPE_NONE)
    {
        rayPayload.MissEmissive = float3(0.0, 0.0, 0.0);
    }
    else if (sceneBuffer.settings.BackgroundType == BACKGROUND_TYPE_GRADIENT)
    {
        const float3 rayDir = normalize(WorldRayDirection());
        const float alpha = 0.5 * (rayDir.y + 1.0);
        const float3 color = (1.0 - alpha) * float3(1.0, 1.0, 1.0) + alpha * float3(0.5, 0.7, 1.0);

        const float strength = max(1.0, sceneBuffer.settings.GradientLightStrength);
        rayPayload.MissEmissive = color * strength;
    }
    else if (sceneBuffer.settings.BackgroundType == BACKGROUND_TYPE_SKYBOX)
    {
        const float3 rayDir = normalize(WorldRayDirection());
        const float4 skyboxColor = cubeTexture.SampleLevel(cubeTextureSampler, rayDir, 0.0);
        rayPayload.MissEmissive = skyboxColor.rgb;
    }
}
