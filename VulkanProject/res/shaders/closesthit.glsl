#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable
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

struct FHWVertex
{
    vec3 Position;
    vec3 Normal;
    vec3 Tangent;
    vec2 TexCoord;
};

layout(buffer_reference, scalar) readonly buffer FVertexBuffer
{
    FHWVertex Vertices[];
};

layout(buffer_reference, scalar) readonly buffer FIndexBuffer
{
    ivec3 Indices[];
};

layout(binding = 5) readonly buffer FMeshInfoBuffer 
{ 
    FMeshInfo MeshInfos[]; 
};

layout(binding = 6) buffer MaterialBuffer
{
    FMaterial Materials[];
};

layout (set = 1, binding = 0) uniform sampler2D uTextures[];

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
    FHWVertex Vertices[3];
    Vertices[0] = VertexBuffer.Vertices[Indices.x];
    Vertices[1] = VertexBuffer.Vertices[Indices.y];
    Vertices[2] = VertexBuffer.Vertices[Indices.z];

    // Output the normal
    const vec3 BarycentricCoords = vec3(1.0 - Attribs.x - Attribs.y, Attribs.x, Attribs.y);

    // Attributes
    const vec3 Position = (Vertices[0].Position * BarycentricCoords.x) + (Vertices[1].Position * BarycentricCoords.y) + (Vertices[2].Position * BarycentricCoords.z);
    const vec3 Normal   = (Vertices[0].Normal   * BarycentricCoords.x) + (Vertices[1].Normal   * BarycentricCoords.y) + (Vertices[2].Normal   * BarycentricCoords.z);
    const vec3 Tangent  = (Vertices[0].Tangent  * BarycentricCoords.x) + (Vertices[1].Tangent  * BarycentricCoords.y) + (Vertices[2].Tangent  * BarycentricCoords.z);
    const vec2 TexCoord = (Vertices[0].TexCoord * BarycentricCoords.x) + (Vertices[1].TexCoord * BarycentricCoords.y) + (Vertices[2].TexCoord * BarycentricCoords.z);

    // Sample Material
    FMaterial Material = Materials[uint(MeshInfo.MaterialIndex)];
    if (Material.AlbedoTexIndex != INVALID_BINDLESS_ID)
    {
        RayPayLoad.HitAlbedo = texture(uTextures[Material.AlbedoTexIndex], TexCoord).rgb;
    }
    else
    {
        RayPayLoad.HitAlbedo = vec3(0.9, 0.9, 0.9);
    }

    if (Material.NormalTexIndex != INVALID_BINDLESS_ID)
    {
        vec3 NormalMap = texture(uTextures[Material.NormalTexIndex], TexCoord).rgb;
        NormalMap = normalize(NormalMap * 2.0 - 1.0); // Transform from [0,1] range to [-1,1]

        const vec3 BiTangent = cross(Normal, Tangent);
        const mat3 TBNMatrix = mat3(Tangent, BiTangent, Normal);
        vec3 MappedNormal = normalize(TBNMatrix * NormalMap);

        if (any(isnan(MappedNormal)) || any(isinf(MappedNormal)))
        {
            MappedNormal = Normal;
        }

        const vec3 WorldNormal = normalize(vec3(MappedNormal * gl_WorldToObjectEXT)); // Transforming the normal to world space
        RayPayLoad.HitNormal = WorldNormal;
    }
    else
    {
        const vec3 WorldNormal = normalize(vec3(Normal * gl_WorldToObjectEXT)); // Transforming the normal to world space
        RayPayLoad.HitNormal = WorldNormal;
    }

    const vec3 WorldPosition = vec3(gl_ObjectToWorldEXT * vec4(Position, 1.0)); // Transforming the position to world space
    RayPayLoad.HitPosition = WorldPosition;
}