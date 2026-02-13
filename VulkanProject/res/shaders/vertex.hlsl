struct VSInput
{
    [[vk::location(0)]] float3 InPosition : POSITION0;
};

#define COMMON_STRUCTS_NO_HW_RAYTRACE
#include "common_structs.hlsli"

[[vk::binding(0, 0)]]
ConstantBuffer<SCameraBuffer> CameraBuffer : register(b0, space0);

float4 main(VSInput InputData) : SV_Position
{
    return mul(mul(CameraBuffer.Projection, CameraBuffer.View), float4(InputData.InPosition, 1.0));
}


