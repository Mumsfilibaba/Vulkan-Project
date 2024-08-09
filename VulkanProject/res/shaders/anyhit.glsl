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

layout(binding = 5) uniform SceneBufferObject 
{
    FSceneSettings Settings;
} uScene;

layout(binding = 6) readonly buffer FMeshInfoBuffer 
{ 
    FMeshInfo MeshInfos[]; 
};

layout(binding = 7) buffer MaterialBuffer
{
    FMaterial Materials[];
};

layout (set = 1, binding = 0) uniform sampler2D uTextures[];

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
    // Attributes
    const vec2 TexCoord = (Vertices[0].TexCoord * BarycentricCoords.x) + (Vertices[1].TexCoord * BarycentricCoords.y) + (Vertices[2].TexCoord * BarycentricCoords.z);

    const uint MaterialIndex = min(uint(MeshInfo.MaterialIndex), uScene.Settings.NumMaterials - 1);
    FMaterial Material = Materials[MaterialIndex];
    if (Material.AlphaMaskTexIndex != INVALID_BINDLESS_ID)
    {
        // Ignore the hit if the alpha mask is below a certain threshold
        float Color = texture(uTextures[Material.AlphaMaskTexIndex], TexCoord).r;
        if (Color < 0.9)
        {
            ignoreIntersectionEXT;
        }
    }
}