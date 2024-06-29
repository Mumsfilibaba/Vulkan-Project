#ifndef PRIMITIVES_GLSL
#define PRIMITIVES_GLSL

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
    // 80-84
    float RefractionRoughness;

    // Padding
    uint Padding0;
    uint Padding1;
    uint Padding2;
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

struct FVertexRT
{
    // 0-16
    vec4 Position;
};

struct FTriangle
{
    // 0-12
    uint Index0;
    uint Index1;
    uint Index2;

    // Padding
    uint Padding0;
};

struct FTriangleMesh
{
    // 0-32
    vec4 BoxMin;
    vec4 BoxMax;
    // 32-44
    uint StartTriangle;
    uint NumTriangles;
    uint MaterialIndex;

    // Padding
    uint Padding0;
};

#endif