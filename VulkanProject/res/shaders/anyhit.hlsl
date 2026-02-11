#include "hw_ray_trace_common.hlsli"
#include "primitives.hlsli"

struct SMeshInfo
{
    uint64_t VertexBufferAddress;
    uint64_t IndexBufferAddress;
    uint     MaterialIndex;
    uint     Flags;
};

struct SSceneBuffer { SSceneSettings Settings; };
[[vk::binding(5)]]
ConstantBuffer<SSceneBuffer> uScene;

[[vk::binding(6)]]
StructuredBuffer<SMeshInfo> MeshInfos;

[[vk::binding(7)]]
StructuredBuffer<SMaterial> Materials;

[[vk::binding(0, 1)]] Texture2D<float4> uTextures[];
[[vk::binding(0, 1)]] SamplerState uTexturesSampler : register(s0, space1);

static const uint VERTEX_STRIDE   = 44;
static const uint TEXCOORD_OFFSET = 36;

[shader("anyhit")]
void main(inout SRayPayLoad rayPayload, in BuiltInTriangleIntersectionAttributes attr)
{
    uint meshInfoOffset = InstanceID();
    uint geometryIndex  = GeometryIndex();
    uint meshInfoIndex  = meshInfoOffset + geometryIndex;
    
    SMeshInfo meshInfo = MeshInfos[meshInfoIndex];

    uint primitiveId = PrimitiveIndex();
    
    uint64_t indexBase = meshInfo.IndexBufferAddress + primitiveId * 12;
    uint idx0 = vk::RawBufferLoad<uint>(indexBase + 0);
    uint idx1 = vk::RawBufferLoad<uint>(indexBase + 4);
    uint idx2 = vk::RawBufferLoad<uint>(indexBase + 8);

    float3 barycentricCoords = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);

    float2 texCoord = float2(0, 0);
    for (uint i = 0; i < 3; i++)
    {
        uint idx = (i == 0) ? idx0 : ((i == 1) ? idx1 : idx2);
        
        float w = (i == 0) ? barycentricCoords.x : ((i == 1) ? barycentricCoords.y : barycentricCoords.z);
        
        uint64_t vbase = meshInfo.VertexBufferAddress + idx * VERTEX_STRIDE;
        float2 vtexCoord = float2(
            vk::RawBufferLoad<float>(vbase + TEXCOORD_OFFSET + 0),
            vk::RawBufferLoad<float>(vbase + TEXCOORD_OFFSET + 4));

        texCoord += vtexCoord * w;
    }

    uint materialIndex = min(meshInfo.MaterialIndex, uScene.Settings.NumMaterials - 1);
    SMaterial material = Materials[materialIndex];

    // Match software quad behavior (front-face only) for opaque surfaces.
    // Keep back-faces for refractive materials so transmission still works.
    if (HitKind() == HIT_KIND_TRIANGLE_BACK_FACE && material.RefractionChance <= 0.0f)
    {
        IgnoreHit();
        return;
    }

    if (material.AlphaMaskTexIndex != INVALID_BINDLESS_ID)
    {
        float alpha = uTextures[material.AlphaMaskTexIndex].SampleLevel(uTexturesSampler, texCoord, 0.0).r;
        if (alpha < 0.9)
        {
            IgnoreHit();
        }
    }
}
