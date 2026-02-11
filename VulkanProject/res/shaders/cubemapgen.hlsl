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

static const float2 invAtan = float2(0.15915494309f, 0.31830988618f); // (1 / 2PI, 1 / PI)

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    const uint3 texCoord = dispatchThreadID;
    const uint  face     = texCoord.z;

    float2 uv = ((float2(texCoord.xy) + 0.5f) / float(pushConstants.cubeMapSize)) * 2.0f - 1.0f;
    uv.y = -uv.y;

    float3 direction = float3(0.0f, 0.0f, 0.0f);
    if (face == 0)       
        direction = normalize(float3( 1.0f,  uv.y, -uv.x)); // +X
    else if (face == 1)  
        direction = normalize(float3(-1.0f,  uv.y,  uv.x)); // -X
    else if (face == 2)  
        direction = normalize(float3( uv.x,  1.0f, -uv.y)); // +Y
    else if (face == 3)  
        direction = normalize(float3( uv.x, -1.0f,  uv.y)); // -Y
    else if (face == 4)  
        direction = normalize(float3( uv.x,  uv.y,  1.0f)); // +Z
    else                 
        direction = normalize(float3(-uv.x,  uv.y, -1.0f)); // -Z

    // Use polar angle for V so +Y maps to top of the panorama.
    float2 panoramaTexCoords;
    panoramaTexCoords.x = atan2(direction.z, direction.x) * invAtan.x + 0.5f;
    panoramaTexCoords.y = acos(clamp(direction.y, -1.0f, 1.0f)) * invAtan.y;

    outCubeTexture[texCoord] = sourceTexture.SampleLevel(sourceSampler, panoramaTexCoords, 0.0f);
}
