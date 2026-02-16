struct PSInput
{
    [[vk::location(0)]] float2 InFragCoord : TEXCOORD0;
};

[[vk::combinedImageSampler]]
[[vk::binding(0, 0)]]
Texture2D<float4> SceneTexture : register(t0, space0);

[[vk::combinedImageSampler]]
[[vk::binding(0, 0)]]
SamplerState SceneTextureSampler : register(s0, space0);

struct SSettingsBuffer
{
    float Exposure;
    uint EnableTonemapping;
    uint Padding0;
    uint Padding1;
};

[[vk::binding(1, 0)]]
ConstantBuffer<SSettingsBuffer> SettingsBuffer : register(b1, space0);

float3 AcesFilm(float3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float3 LinearToSrgb(float3 LinearColor)
{
    float3 SrgbColor = 0.0.xxx;
    [unroll]
    for (int i = 0; i < 3; ++i)
    {
        if (LinearColor[i] <= 0.0031308)
        {
            SrgbColor[i] = 12.92 * LinearColor[i];
        }
        else
        {
            SrgbColor[i] = 1.055 * pow(LinearColor[i], 1.0 / 2.4) - 0.055;
        }
    }

    return SrgbColor;
}

float4 main(PSInput InputData) : SV_Target0
{
    float3 Color = SceneTexture.SampleLevel(SceneTextureSampler, InputData.InFragCoord, 0.0).rgb;
    if (SettingsBuffer.EnableTonemapping != 0)
    {
        Color *= SettingsBuffer.Exposure;
        Color = AcesFilm(Color);
        Color = LinearToSrgb(Color);
    }

    return float4(Color, 1.0);
}


