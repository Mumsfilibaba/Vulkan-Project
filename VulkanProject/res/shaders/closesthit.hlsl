#include "hw_ray_trace_common.hlsli"
#include "primitives.hlsli"

struct SMeshInfo
{
    uint64_t VertexBufferAddress;
    uint64_t IndexBufferAddress;
    uint     MaterialIndex;
    uint     Flags;
};

[[vk::binding(6)]]
StructuredBuffer<SMeshInfo> MeshInfos;

// Must match C++ SVertex packing in Model.h exactly.
static const uint VERTEX_STRIDE   = 44;
static const uint POSITION_OFFSET = 0;
static const uint NORMAL_OFFSET   = 12;
static const uint TANGENT_OFFSET  = 24;
static const uint TEXCOORD_OFFSET = 36;

[shader("closesthit")]
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
    rayPayload.HitBarycentrics = barycentricCoords;

    float3 position = float3(0, 0, 0);
    float3 normal   = float3(0, 0, 0);
    float3 tangent  = float3(0, 0, 0);
    float2 texCoord = float2(0, 0);

    float3 trianglePositions[3];
    float3 triangleNormals[3];
    float2 triangleTexCoords[3];

    uint indices[3] = { idx0, idx1, idx2 };
    for (uint i = 0; i < 3; i++)
    {
        uint64_t vbase = meshInfo.VertexBufferAddress + indices[i] * VERTEX_STRIDE;
        
        float w = (i == 0) ? barycentricCoords.x : ((i == 1) ? barycentricCoords.y : barycentricCoords.z);
        
        float3 vpos = float3(
            vk::RawBufferLoad<float>(vbase + POSITION_OFFSET + 0),
            vk::RawBufferLoad<float>(vbase + POSITION_OFFSET + 4),
            vk::RawBufferLoad<float>(vbase + POSITION_OFFSET + 8));
        
        float3 vnormal = float3(
            vk::RawBufferLoad<float>(vbase + NORMAL_OFFSET + 0),
            vk::RawBufferLoad<float>(vbase + NORMAL_OFFSET + 4),
            vk::RawBufferLoad<float>(vbase + NORMAL_OFFSET + 8));
        
        float3 vtangent = float3(
            vk::RawBufferLoad<float>(vbase + TANGENT_OFFSET + 0),
            vk::RawBufferLoad<float>(vbase + TANGENT_OFFSET + 4),
            vk::RawBufferLoad<float>(vbase + TANGENT_OFFSET + 8));

        float2 vtexCoord = float2(
            vk::RawBufferLoad<float>(vbase + TEXCOORD_OFFSET + 0),
            vk::RawBufferLoad<float>(vbase + TEXCOORD_OFFSET + 4));
        
        trianglePositions[i] = vpos;
        triangleNormals[i]   = vnormal;
        triangleTexCoords[i] = vtexCoord;

        position += vpos * w;
        normal   += vnormal * w;
        tangent  += vtangent * w;
        texCoord += vtexCoord * w;
    }

    float3x4 objectToWorld = ObjectToWorld3x4();
    rayPayload.HitNormal = normalize(mul((float3x3)objectToWorld, normal));

    if (any(isnan(rayPayload.HitNormal)) || any(isinf(rayPayload.HitNormal)))
    {
        rayPayload.HitNormal = normalize(normal);
    }

    rayPayload.HitTangent = normalize(mul((float3x3)objectToWorld, tangent));
    if (any(isnan(rayPayload.HitTangent)) || any(isinf(rayPayload.HitTangent)))
    {
        rayPayload.HitTangent = normalize(tangent);
    }

    float3 worldRayDirection = normalize(WorldRayDirection());
    rayPayload.bFromInside = dot(rayPayload.HitNormal, worldRayDirection) > 0.0 ? 1u : 0u;
    if (rayPayload.bFromInside)
    {
        rayPayload.HitNormal  = -rayPayload.HitNormal;
        rayPayload.HitTangent = -rayPayload.HitTangent;
    }

    rayPayload.HitPosition      = mul(objectToWorld, float4(position, 1.0)).xyz;
    rayPayload.HitTexCoord      = texCoord;
    rayPayload.HitMaterialIndex = meshInfo.MaterialIndex;
    rayPayload.HitT             = RayTCurrent();
}
