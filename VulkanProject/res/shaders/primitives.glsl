#ifndef PRIMITIVES_GLSL
#define PRIMITIVES_GLSL

#define INVALID_BINDLESS_ID uint(-1)

struct FMaterial
{
    // 0-16
    vec4 AlbedoColor;
    // 16-32
    vec4 EmissiveColor;
    // 32-48
    vec4 SpecularColor;
    // 48-64
    vec4 AbsorbtionColor;
    // 64-80
    float SpecularChance;
    float SpecularRoughness;
    float IncidenceOfRefraction;
    float RefractionChance;
    // 80-88
    float RefractionRoughness;
    // 84-88
    uint AlbedoTexIndex;

    // Padding
    uint Padding0;
    uint Padding1;
};

struct FQuad
{
    // 0-16
    vec4 Position;
    // 16-32
    vec4 Edge0;
    // 32-48
    vec4 Edge1;
    // 48-52
    uint MaterialIndex;

    // Padding
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

struct FSphere
{
    // 0-16
    vec4 PositionAndRadius;
    // 16-20
    uint MaterialIndex;

    // Padding
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

struct FVertex
{
    // 0-16
    vec4 Position;
};

struct FVertexEx
{
    // 0-16
    vec4 Normal;
    // 16-32
    vec4 TexCoords;
};

struct FTriangle
{
    // 0-16
    uint Index0;
    uint Index1;
    uint Index2;
    uint MaterialIndex;
};

struct FMesh
{
    // 0-8
    uint BoundingBoxIndex;
    uint MaterialIndex;
};

#endif