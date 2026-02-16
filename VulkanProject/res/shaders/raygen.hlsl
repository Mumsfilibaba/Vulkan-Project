#include "common_structs.hlsli"
#include "random.hlsli"
#include "primitives.hlsli"

#define RAY_OFFSET 0.001
#define MAX_DEPTH 1024

#define VIEW_MODE_RENDER 0
#define VIEW_MODE_NORMALS 1
#define VIEW_MODE_GEOMETRIC_NORMALS 2
#define VIEW_MODE_TANGENTS 3
#define VIEW_MODE_ALBEDO 4
#define VIEW_MODE_BARYCENTRICS 5
#define VIEW_MODE_TEXCOORDS 6

#define ENABLE_RUSSIAN_ROULETTE 1
#define HW_RAY_FLAGS RAY_FLAG_CULL_BACK_FACING_TRIANGLES

[[vk::binding(1)]] RaytracingAccelerationStructure AccelerationStructure;
[[vk::binding(2)]] RWTexture2D<float4> OutputTexture;
[[vk::binding(3)]] RWTexture2D<float4> PreviousFrameTexture;

struct SSceneBuffer
{
    SSceneSettings Settings;
};

[[vk::binding(14)]]
ConstantBuffer<SCameraBuffer> Camera;

[[vk::binding(15)]]
ConstantBuffer<SRandomBuffer> Random;

[[vk::binding(16)]]
ConstantBuffer<SSceneBuffer> Scene;

[[vk::binding(6)]]
StructuredBuffer<SMaterial> MaterialBuffer;

[[vk::binding(0)]] Texture2D<float4> Textures[];
[[vk::binding(0)]] SamplerState TexturesSampler : register(s0);

#include "shading.hlsli"
#include "trace_common.hlsli"
#include "trace_backend_hardware.hlsli"

[shader("raygeneration")]
void main()
{
    const bool WritePrimary = (Random.FrameIndex & 1) == 0;

    uint3 LaunchId   = DispatchRaysIndex();
    uint3 LaunchSize = DispatchRaysDimensions();
    uint  RandomSeed = InitRandom(LaunchId.xy, LaunchSize.x, Random.FrameIndex);

    float2 Jitter = float2(0.0, 0.0);
    if (Scene.Settings.ViewMode == VIEW_MODE_RENDER)
    {
        Jitter = float2(NextRandom(RandomSeed), NextRandom(RandomSeed)) - 0.5;
    }
    float2 PixelCenter = float2(LaunchId.xy) + float2(0.5, 0.5);

    float3 Origin    = Camera.Position.xyz;
    float3 Target    = CalculateFilmTarget(Origin, Camera.Forward.xyz, Camera.FieldOfViewDegrees, PixelCenter, float2(LaunchSize.xy), Jitter);
    float3 Direction = normalize(Target - Origin);

    SRayDesc RayDesc;
    RayDesc.Origin    = Origin;
    RayDesc.Direction = Direction;
    RayDesc.MinT      = 0.001;
    RayDesc.MaxT      = 10000.0;

    const uint NumBounces = min(Scene.Settings.NumBounces, MAX_DEPTH);
    bool Accumulate = false;
    
    float3 SampleColor = EvaluateViewModeColor(Scene.Settings.ViewMode, RayDesc, RandomSeed, NumBounces, Accumulate);
    WriteViewModeOutput(WritePrimary, LaunchId.xy, SampleColor, Accumulate, Random.FrameIndex, OutputTexture, PreviousFrameTexture);
}


