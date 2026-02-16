struct VSInput
{
    [[vk::location(0)]] float3 InPosition : POSITION0;
};

#include "primitives.hlsli"

#define COMMON_STRUCTS_NO_HW_RAYTRACE
#include "common_structs.hlsli"

[[vk::binding(0, 0)]]
ConstantBuffer<SCameraBuffer> CameraBuffer : register(b0, space0);

[[vk::binding(1, 0)]]
StructuredBuffer<SMesh> Meshes : register(t1, space0);

float4 main(VSInput InputData, uint InstanceIndex : SV_InstanceID) : SV_Position
{
    const float4x4 LocalToWorld = Meshes[InstanceIndex].LocalToWorld;
    return mul(mul(mul(CameraBuffer.Projection, CameraBuffer.View), LocalToWorld), float4(InputData.InPosition, 1.0));
}


