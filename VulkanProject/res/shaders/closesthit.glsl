#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "hw_ray_trace_common.glsl"

struct FMeshInfo
{
    uint64_t VertexBufferAddress;
    uint64_t IndexBufferAddress;
};

struct FVertex
{
    vec3 Position;
    vec3 Normal;
    vec2 TexCoord;
};

layout(buffer_reference, scalar) readonly buffer FVertexBuffer
{
    FVertex Vertices[];
};

layout(buffer_reference, scalar) readonly buffer FIndexBuffer
{
    ivec3 Indices[];
};

layout(binding = 4) readonly buffer FMeshInfoBuffer 
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

    // Computing the coordinates of the hit position
    const vec3 Position      = (Vertices[0].Position * BarycentricCoords.x) + (Vertices[1].Position * BarycentricCoords.y) + (Vertices[2].Position * BarycentricCoords.z);
    const vec3 WorldPosition = vec3(gl_ObjectToWorldEXT * vec4(Position, 1.0)); // Transforming the position to world space
    RayPayLoad.HitPosition = WorldPosition;

    // Output Normal
    const vec3 Normal      = (Vertices[0].Normal * BarycentricCoords.x) + (Vertices[1].Normal * BarycentricCoords.y) + (Vertices[2].Normal * BarycentricCoords.z);
    const vec3 WorldNormal = normalize(vec3(Normal * gl_WorldToObjectEXT)); // Transforming the normal to world space
    RayPayLoad.HitNormal = WorldNormal;

    // Output Color
    RayPayLoad.HitAlbedo = vec3(0.9, 0.9, 0.9);
}