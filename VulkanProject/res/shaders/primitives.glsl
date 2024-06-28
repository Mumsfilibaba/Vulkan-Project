#ifndef PRIMITIVES_GLSL
#define PRIMITIVES_GLSL

struct FMaterial
{
    vec4  AlbedoColor;
    vec4  EmissiveColor;
    vec4  SpecularColor;
    vec4  AbsorbtionColor;
    float SpecularChance;
    float SpecularRoughness;
    float IncidenceOfRefraction;
    float RefractionChance;
    float RefractionRoughness;
    uint  Padding0;
    uint  Padding1;
    uint  Padding2;
};

struct FQuad
{
    vec4 Position;
    vec4 Edge0;
    vec4 Edge1;
    uint MaterialIndex;
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

struct FSphere
{
    vec4 PositionAndRadius;
    uint MaterialIndex;
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

struct FPlane 
{
    vec4 NormalAndDistance;
    uint MaterialIndex;
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

struct FVertexRT
{
    vec4 Position;
};

struct FTriangle
{
    uint Index0;
    uint Index1;
    uint Index2;
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