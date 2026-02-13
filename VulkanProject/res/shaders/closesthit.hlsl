#include "common_structs.hlsli"
#include "primitives.hlsli"

struct SMeshInfo
{
    uint64_t VertexBufferAddress;
    uint64_t IndexBufferAddress;
    uint     MaterialIndex;
    uint     Flags;
};

[[vk::binding(5)]]
StructuredBuffer<SMeshInfo> MeshInfos;

// Must match C++ SVertex packing in Model.h exactly.
static const uint VERTEX_STRIDE   = 44;
static const uint POSITION_OFFSET = 0;
static const uint NORMAL_OFFSET   = 12;
static const uint TANGENT_OFFSET  = 24;
static const uint TEXCOORD_OFFSET = 36;

[shader("closesthit")]
void main(inout SRayPayload RayPayload, in BuiltInTriangleIntersectionAttributes attr)
{
    uint MeshInfoOffset = InstanceID();
    uint GeometryIdx = GeometryIndex();
    uint MeshInfoIndex  = MeshInfoOffset + GeometryIdx;
    
    SMeshInfo MeshInfo = MeshInfos[MeshInfoIndex];

    uint PrimitiveIdLocal = PrimitiveIndex();
    uint64_t IndexBase = MeshInfo.IndexBufferAddress + PrimitiveIdLocal * 12;
    
    uint Idx0 = vk::RawBufferLoad<uint>(IndexBase + 0);
    uint Idx1 = vk::RawBufferLoad<uint>(IndexBase + 4);
    uint Idx2 = vk::RawBufferLoad<uint>(IndexBase + 8);

    float3 BarycentricCoords = float3(1.0 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    RayPayload.HitBarycentrics = BarycentricCoords;

    float3 Position = float3(0, 0, 0);
    float3 Normal   = float3(0, 0, 0);
    float3 Tangent  = float3(0, 0, 0);
    float2 TexCoord = float2(0, 0);

    float3 TrianglePositions[3];
    float3 TriangleNormals[3];
    float2 TriangleTexCoords[3];

    uint IndicesLocal[3] = { Idx0, Idx1, Idx2 };
    for (uint i = 0; i < 3; i++)
    {
        uint64_t VBase = MeshInfo.VertexBufferAddress + IndicesLocal[i] * VERTEX_STRIDE;
        
        float Weight = (i == 0) ? BarycentricCoords.x : ((i == 1) ? BarycentricCoords.y : BarycentricCoords.z);
        
        float3 VPos = float3(
            vk::RawBufferLoad<float>(VBase + POSITION_OFFSET + 0),
            vk::RawBufferLoad<float>(VBase + POSITION_OFFSET + 4),
            vk::RawBufferLoad<float>(VBase + POSITION_OFFSET + 8));
        
        float3 VNormal = float3(
            vk::RawBufferLoad<float>(VBase + NORMAL_OFFSET + 0),
            vk::RawBufferLoad<float>(VBase + NORMAL_OFFSET + 4),
            vk::RawBufferLoad<float>(VBase + NORMAL_OFFSET + 8));
        
        float3 VTangent = float3(
            vk::RawBufferLoad<float>(VBase + TANGENT_OFFSET + 0),
            vk::RawBufferLoad<float>(VBase + TANGENT_OFFSET + 4),
            vk::RawBufferLoad<float>(VBase + TANGENT_OFFSET + 8));

        float2 VTexCoord = float2(
            vk::RawBufferLoad<float>(VBase + TEXCOORD_OFFSET + 0),
            vk::RawBufferLoad<float>(VBase + TEXCOORD_OFFSET + 4));
        
        TrianglePositions[i] = VPos;
        TriangleNormals[i]   = VNormal;
        TriangleTexCoords[i] = VTexCoord;

        Position += VPos * Weight;
        Normal   += VNormal * Weight;
        Tangent  += VTangent * Weight;
        TexCoord += VTexCoord * Weight;
    }

    float3x4 ObjectToWorld = ObjectToWorld3x4();
    RayPayload.HitNormal = normalize(mul((float3x3)ObjectToWorld, Normal));

    if (any(isnan(RayPayload.HitNormal)) || any(isinf(RayPayload.HitNormal)))
    {
        RayPayload.HitNormal = normalize(Normal);
    }

    RayPayload.HitTangent = normalize(mul((float3x3)ObjectToWorld, Tangent));
    if (any(isnan(RayPayload.HitTangent)) || any(isinf(RayPayload.HitTangent)))
    {
        RayPayload.HitTangent = normalize(Tangent);
    }

    float3 WorldRayDirectionValue = normalize(WorldRayDirection());
    RayPayload.FromInside = dot(RayPayload.HitNormal, WorldRayDirectionValue) > 0.0 ? 1u : 0u;
    if (RayPayload.FromInside)
    {
        RayPayload.HitNormal  = -RayPayload.HitNormal;
        RayPayload.HitTangent = -RayPayload.HitTangent;
    }

    RayPayload.HitPosition      = mul(ObjectToWorld, float4(Position, 1.0)).xyz;
    RayPayload.HitTexCoord      = TexCoord;
    RayPayload.HitMaterialIndex = MeshInfo.MaterialIndex;
    RayPayload.HitT             = RayTCurrent();
}



