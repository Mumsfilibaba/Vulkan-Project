#define NUM_THREADS 16

struct SPushConstant
{
    uint cubeMapSize;
};

[[vk::push_constant]]
ConstantBuffer<SPushConstant> pushConstants;

[[vk::combinedImageSampler]]
[[vk::binding(0, 0)]]
Texture2D<float4> sourceTexture : register(t0, space0);

[[vk::combinedImageSampler]]
[[vk::binding(0, 0)]]
SamplerState sourceSampler : register(s0, space0);

[[vk::binding(1, 0)]]
RWTexture2DArray<float4> outCubeTexture : register(u1, space0);

static const float2 invAtan = float2(0.1591f, 0.3183f);

static const float3x3 rotateUv[6] =
{
    float3x3( 0.0,  0.0,  1.0,
              0.0, -1.0,  0.0,
             -1.0,  0.0,  0.0),
    float3x3( 0.0,  0.0, -1.0,
              0.0, -1.0,  0.0,
              1.0,  0.0,  0.0),
    float3x3( 1.0,  0.0,  0.0,
              0.0,  0.0,  1.0,
              0.0,  1.0,  0.0),
    float3x3( 1.0,  0.0,  0.0,
              0.0,  0.0, -1.0,
              0.0, -1.0,  0.0),
    float3x3( 1.0,  0.0,  0.0,
              0.0, -1.0,  0.0,
              0.0,  0.0,  1.0),
    float3x3(-1.0,  0.0,  0.0,
              0.0, -1.0,  0.0,
              0.0,  0.0, -1.0)
};

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    const uint3 texCoord = dispatchThreadID;
    const float2 uvs = (float2(texCoord.xy) / float(pushConstants.cubeMapSize));
    float3 direction = float3(uvs - 0.5, 0.5);

    direction = normalize(mul(direction, rotateUv[texCoord.z]));

    const float2 panoramaTexCoords = float2(atan2(direction.x, direction.z), acos(direction.y)) * invAtan;
    outCubeTexture[uint3(dispatchThreadID)] = sourceTexture.SampleLevel(sourceSampler, panoramaTexCoords, 0.0);
}
