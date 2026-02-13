struct SPushConstant
{
    float4 Color;
};

[[vk::push_constant]]
ConstantBuffer<SPushConstant> PushConstants;

float4 main() : SV_Target0
{
    return float4(PushConstants.Color.rgb, 1.0);
}


