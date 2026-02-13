#include "common_structs.hlsli"
#include "primitives.hlsli"

struct SMeshInfo
{
    uint64_t VertexBufferAddress;
    uint64_t IndexBufferAddress;
    uint     MaterialIndexLocal;
    uint     Flags;
};

struct SSceneBuffer
{
    SSceneSettings Settings;
};

[[vk::binding(16)]]
ConstantBuffer<SSceneBuffer> Scene;

[[vk::binding(5)]]
StructuredBuffer<SMeshInfo> MeshInfos;

[[vk::binding(6)]]
StructuredBuffer<SMaterial> Materials;

[[vk::binding(0)]] Texture2D<float4> Textures[];
[[vk::binding(0)]] SamplerState TexturesSampler : register(s0);

static const uint VERTEX_STRIDE   = 44;
static const uint TEXCOORD_OFFSET = 36;

[shader("anyhit")]
void main(inout SRayPayload RayPayload, in BuiltInTriangleIntersectionAttributes attr)
{
    uint MeshInfoOffset = InstanceID();
    uint GeometryIdx = GeometryIndex();
    uint MeshInfoIndex  = MeshInfoOffset + GeometryIdx;
    
    SMeshInfo MeshInfo = MeshInfos[MeshInfoIndex];

    uint     PrimitiveIdLocal = PrimitiveIndex();
    uint64_t IndexBase   = MeshInfo.IndexBufferAddress + PrimitiveIdLocal * 12;

    uint Idx0 = vk::RawBufferLoad<uint>(IndexBase + 0);
    uint Idx1 = vk::RawBufferLoad<uint>(IndexBase + 4);
    uint Idx2 = vk::RawBufferLoad<uint>(IndexBase + 8);

    float3 BarycentricCoords = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);

    float2 TexCoord = float2(0, 0);
    for (uint i = 0; i < 3; i++)
    {
        uint Idx = (i == 0) ? Idx0 : ((i == 1) ? Idx1 : Idx2);
        
        float Weight = (i == 0) ? BarycentricCoords.x : ((i == 1) ? BarycentricCoords.y : BarycentricCoords.z);
        
        uint64_t VBase = MeshInfo.VertexBufferAddress + Idx * VERTEX_STRIDE;
        float2 VTexCoord = float2(
            vk::RawBufferLoad<float>(VBase + TEXCOORD_OFFSET + 0),
            vk::RawBufferLoad<float>(VBase + TEXCOORD_OFFSET + 4));

        TexCoord += VTexCoord * Weight;
    }

    uint MaterialIndexLocal = min(MeshInfo.MaterialIndexLocal, Scene.Settings.NumMaterials - 1);
    SMaterial MaterialLocal = Materials[MaterialIndexLocal];

    if (MaterialLocal.AlphaMaskTexIndex != INVALID_BINDLESS_ID)
    {
        float Alpha = Textures[MaterialLocal.AlphaMaskTexIndex].SampleLevel(TexturesSampler, TexCoord, 0.0).r;
        if (Alpha < 0.9)
        {
            IgnoreHit();
        }
    }
}




