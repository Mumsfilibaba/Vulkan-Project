#ifndef PRIMITIVES_HLSLI
#define PRIMITIVES_HLSLI

#define INVALID_BINDLESS_ID 0xFFFFFFFFu

struct SMaterial
{
    float4 AlbedoColor;
    float4 EmissiveColor;
    float4 SpecularColor;
    float4 AbsorbtionColor;
    float SpecularChance;
    float SpecularRoughness;
    float IncidenceOfRefraction;
    float RefractionChance;
    float RefractionRoughness;
    uint AlbedoTexIndex;
    uint NormalTexIndex;
    uint AlphaMaskTexIndex;
    uint MetallicTexIndex;
    uint RoughnessTexIndex;
    uint Padding0;
    uint Padding1;
};

struct SQuad
{
    float4 Position;
    float4 Edge0;
    float4 Edge1;
    uint MaterialIndex;
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

struct SSphere
{
    float4 PositionAndRadius;
    uint MaterialIndex;
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

struct SVertexPosition
{
    float3 Position;
};

struct SVertex
{
    float3 Position;
    float3 Normal;
    float3 Tangent;
    float2 TexCoord;
};

struct STriangle
{
    int MaterialIndex;
};

struct SMesh
{
    uint BoundingBoxIndex;
};

struct SHitInfo
{
    float2 BaryCentrics;
    float Dist;
};

#endif
