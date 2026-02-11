struct VSOutput
{
    float4 position : SV_Position;
    [[vk::location(0)]] float2 fragCoord : TEXCOORD0;
};

VSOutput main(uint vertexIndex : SV_VertexID)
{
    VSOutput output;

    static const float2 positions[3] =
    {
        float2(-1.0, -1.0),
        float2(3.0, -1.0),
        float2(-1.0, 3.0)
    };

    output.position  = float4(positions[vertexIndex], 0.0, 1.0);
    output.fragCoord = (positions[vertexIndex] + float2(1.0, 1.0)) * 0.5;
    return output;
}
