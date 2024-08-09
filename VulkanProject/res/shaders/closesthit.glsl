#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "hw_ray_trace_common.glsl"
#include "primitives.glsl"

struct FMeshInfo
{
    uint64_t VertexBufferAddress;
    uint64_t IndexBufferAddress;
    uint64_t MaterialIndex;
};

layout(buffer_reference, scalar) readonly buffer FVertexBuffer
{
    FVertex Vertices[];
};

layout(buffer_reference, scalar) readonly buffer FIndexBuffer
{
    ivec3 Indices[];
};

layout(binding = 6) readonly buffer FMeshInfoBuffer 
{ 
    FMeshInfo MeshInfos[]; 
};

layout(location = 0) rayPayloadInEXT FRayPayLoad RayPayLoad;

hitAttributeEXT vec2 Attribs;

void main()
{
    const uint GeometryIndex = gl_GeometryIndexEXT;
    FMeshInfo MeshInfo = MeshInfos[GeometryIndex];

    // Retrieve Buffers
    FIndexBuffer  IndexBuffer  = FIndexBuffer(MeshInfo.IndexBufferAddress);
    FVertexBuffer VertexBuffer = FVertexBuffer(MeshInfo.VertexBufferAddress);

    // The primtive index
    const ivec3 Indices = IndexBuffer.Indices[gl_PrimitiveID];

    // Gather vertices
    FVertex Vertices[3];
    Vertices[0] = VertexBuffer.Vertices[Indices.x];
    Vertices[1] = VertexBuffer.Vertices[Indices.y];
    Vertices[2] = VertexBuffer.Vertices[Indices.z];

    // Output the normal
    const vec3 BarycentricCoords = vec3(1.0 - Attribs.x - Attribs.y, Attribs.x, Attribs.y);
    RayPayLoad.HitBarycentrics = BarycentricCoords;
    
    // Attributes
    const vec3 Position = (Vertices[0].Position * BarycentricCoords.x) + (Vertices[1].Position * BarycentricCoords.y) + (Vertices[2].Position * BarycentricCoords.z);
    const vec3 Normal   = (Vertices[0].Normal   * BarycentricCoords.x) + (Vertices[1].Normal   * BarycentricCoords.y) + (Vertices[2].Normal   * BarycentricCoords.z);
    const vec3 Tangent  = (Vertices[0].Tangent  * BarycentricCoords.x) + (Vertices[1].Tangent  * BarycentricCoords.y) + (Vertices[2].Tangent  * BarycentricCoords.z);
    const vec2 TexCoord = (Vertices[0].TexCoord * BarycentricCoords.x) + (Vertices[1].TexCoord * BarycentricCoords.y) + (Vertices[2].TexCoord * BarycentricCoords.z);

    RayPayLoad.HitNormal = normalize(vec3(Normal * gl_WorldToObjectEXT));
    if (any(isnan(RayPayLoad.HitNormal)) || any(isinf(RayPayLoad.HitNormal)))
    {
        RayPayLoad.HitNormal = Normal;
    }

    RayPayLoad.HitTangent = normalize(vec3(Tangent * gl_WorldToObjectEXT));
    if (any(isnan(RayPayLoad.HitTangent)) || any(isinf(RayPayLoad.HitTangent)))
    {
        RayPayLoad.HitTangent = Tangent;
    }

    RayPayLoad.HitPosition      = vec3(gl_ObjectToWorldEXT * vec4(Position, 1.0)); // Transforming the position to world space;
    RayPayLoad.HitTexCoord      = TexCoord;
    RayPayLoad.HitMaterialIndex = uint(MeshInfo.MaterialIndex);
    RayPayLoad.HitT             = gl_HitTEXT; 
}