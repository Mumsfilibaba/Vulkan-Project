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

float4 main(VSInput input) : SV_Position
{
    return mul(mul(cameraBuffer.projection, cameraBuffer.view), float4(input.inPosition, 1.0));
}
