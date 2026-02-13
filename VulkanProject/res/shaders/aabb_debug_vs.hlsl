struct VSInput
{
    [[vk::location(0)]] float3 InPosition : POSITION0;
};

#define COMMON_STRUCTS_NO_HW_RAYTRACE
#include "common_structs.hlsli"

[[vk::binding(0, 0)]]
ConstantBuffer<SCameraBuffer> CameraBuffer : register(b0, space0);

struct SPushConstant
{
    float4 color;
};

[[vk::push_constant]]
ConstantBuffer<SPushConstant> PushConstants;

[[vk::binding(1, 0)]]
StructuredBuffer<float4x4> InstanceMatrices : register(t1, space0);

float4 main(VSInput InputData, uint InstanceIndex : SV_InstanceID) : SV_Position
{
    float4x4 TransformMatrix = InstanceMatrices[InstanceIndex];
    return mul(mul(mul(CameraBuffer.Projection, CameraBuffer.View), TransformMatrix), float4(InputData.InPosition, 1.0));
}


