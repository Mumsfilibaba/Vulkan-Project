struct PSInput
{
    [[vk::location(0)]] float2 inFragCoord : TEXCOORD0;
};

[[vk::combinedImageSampler]]
[[vk::binding(0, 0)]]
Texture2D<float4> sceneTexture : register(t0, space0);

[[vk::combinedImageSampler]]
[[vk::binding(0, 0)]]
SamplerState sceneTextureSampler : register(s0, space0);

struct SSettingsBuffer
{
    float exposure;
    uint padding0;
    uint padding1;
    uint padding2;
};

[[vk::binding(1, 0)]]
ConstantBuffer<SSettingsBuffer> settingsBuffer : register(b1, space0);

float3 AcesFilm(float3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float3 LinearToSrgb(float3 linearColor)
{
    float3 srgbColor = 0.0.xxx;
    [unroll]
    for (int i = 0; i < 3; ++i)
    {
        if (linearColor[i] <= 0.0031308)
        {
            srgbColor[i] = 12.92 * linearColor[i];
        }
        else
        {
            srgbColor[i] = 1.055 * pow(linearColor[i], 1.0 / 2.4) - 0.055;
        }
    }

    return srgbColor;
}

float4 main(PSInput input) : SV_Target0
{
    float3 color = sceneTexture.SampleLevel(sceneTextureSampler, input.inFragCoord, 0.0).rgb;
    color *= settingsBuffer.exposure;
    color = AcesFilm(color);
    color = LinearToSrgb(color);
    return float4(color, 1.0);
}
