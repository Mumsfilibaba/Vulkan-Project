#define NUM_THREADS 16

struct SPushConstant
{
    uint CubeMapSize;
};

[[vk::push_constant]]
ConstantBuffer<SPushConstant> PushConstants;

[[vk::combinedImageSampler]]
[[vk::binding(0, 0)]]
Texture2D<float4> SourceTexture : register(t0, space0);

[[vk::combinedImageSampler]]
[[vk::binding(0, 0)]]
SamplerState SourceSampler : register(s0, space0);

[[vk::binding(1, 0)]]
[[vk::image_format("rgba16f")]]
RWTexture2DArray<float4> OutCubeTexture : register(u1, space0);

static const float2 InvAtan = float2(0.15915494309f, 0.31830988618f); // (1 / 2PI, 1 / PI)

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    const uint3 TexCoord = DispatchThreadID;
    const uint  Face     = TexCoord.z;

    float2 Uv = ((float2(TexCoord.xy) + 0.5f) / float(PushConstants.CubeMapSize)) * 2.0f - 1.0f;
    Uv.y = -Uv.y;

    float3 Direction = float3(0.0f, 0.0f, 0.0f);
    if (Face == 0)
    {
        Direction = normalize(float3( 1.0f,  Uv.y, -Uv.x)); // +X
    }    
    else if (Face == 1)  
    {
        Direction = normalize(float3(-1.0f,  Uv.y,  Uv.x)); // -X
    }
    else if (Face == 2)  
    {
        Direction = normalize(float3( Uv.x,  1.0f, -Uv.y)); // +Y
    }
    else if (Face == 3)  
    {
        Direction = normalize(float3( Uv.x, -1.0f,  Uv.y)); // -Y
    }
    else if (Face == 4)  
    {
        Direction = normalize(float3( Uv.x,  Uv.y,  1.0f)); // +Z
    }
    else                 
    {
        Direction = normalize(float3(-Uv.x,  Uv.y, -1.0f)); // -Z
    }

    // Use polar angle for V so +Y maps to top of the panorama.
    float2 PanoramaTexCoords;
    PanoramaTexCoords.x = atan2(Direction.z, Direction.x) * InvAtan.x + 0.5f;
    PanoramaTexCoords.y = acos(clamp(Direction.y, -1.0f, 1.0f)) * InvAtan.y;

    OutCubeTexture[TexCoord] = SourceTexture.SampleLevel(SourceSampler, PanoramaTexCoords, 0.0f);
}


