struct VSOutput
{
    float4 Position : SV_Position;
    [[vk::location(0)]] float2 FragCoord : TEXCOORD0;
};

VSOutput main(uint VertexIndex : SV_VertexID)
{
    VSOutput OutputData;

    static const float2 Positions[3] =
    {
        float2(-1.0, -1.0),
        float2(3.0, -1.0),
        float2(-1.0, 3.0)
    };

    OutputData.Position  = float4(Positions[VertexIndex], 0.0, 1.0);
    OutputData.FragCoord = (Positions[VertexIndex] + float2(1.0, 1.0)) * 0.5;
    return OutputData;
}

