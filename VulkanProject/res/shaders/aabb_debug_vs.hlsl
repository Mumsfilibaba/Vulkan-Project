struct VSInput
{
    [[vk::location(0)]] float3 inPosition : POSITION0;
};

struct SCameraBuffer
{
    float4x4 projection;
    float4x4 view;
    float4x4 inverseProjection;
    float4x4 inverseView;
    float4 position;
    float4 forward;
    float fieldOfViewDegrees;
    uint padding0;
    uint padding1;
    uint padding2;
};

[[vk::binding(0, 0)]]
ConstantBuffer<SCameraBuffer> cameraBuffer : register(b0, space0);

struct SPushConstant
{
    float4 color;
};

[[vk::push_constant]]
ConstantBuffer<SPushConstant> pushConstants;

[[vk::binding(1, 0)]]
StructuredBuffer<float4x4> instanceMatrices : register(t1, space0);

float4 main(VSInput input, uint instanceIndex : SV_InstanceID) : SV_Position
{
    float4x4 transformMatrix = instanceMatrices[instanceIndex];
    return mul(mul(mul(cameraBuffer.projection, cameraBuffer.view), transformMatrix), float4(input.inPosition, 1.0));
}
