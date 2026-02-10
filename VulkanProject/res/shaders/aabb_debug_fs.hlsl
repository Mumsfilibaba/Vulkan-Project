struct SPushConstant
{
    float4 color;
};

[[vk::push_constant]]
ConstantBuffer<SPushConstant> pushConstants;

float4 main() : SV_Target0
{
    return float4(pushConstants.color.rgb, 1.0);
}
