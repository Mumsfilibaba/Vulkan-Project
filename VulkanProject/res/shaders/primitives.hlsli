#ifndef PRIMITIVES_HLSLI
#define PRIMITIVES_HLSLI

#define INVALID_BINDLESS_ID 0xFFFFFFFFu

struct SMaterial
{
    // 0-16
    float4 AlbedoColor;
    // 16-32
    float4 EmissiveColor;
    // 32-48
    float4 SpecularColor;
    // 48-64
    float4 AbsorbtionColor;
    // 64-80
    float SpecularChance;
    float SpecularRoughness;
    float IncidenceOfRefraction;
    float RefractionChance;
    // 80-84
    float RefractionRoughness;
    // 84-92
    uint AlbedoTexIndex;
    uint NormalTexIndex;
    uint AlphaMaskTexIndex;
    // 92-100
    uint MetallicTexIndex;
    uint RoughnessTexIndex;
    // Padding
    uint Padding0;
    uint Padding1;
};

struct SQuad
{
    // 0-16
    float4 Position;
    // 16-32
    float4 Edge0;
    // 32-48
    float4 Edge1;
    // 48-52
    uint MaterialIndex;
    // Padding
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

struct SSphere
{
    // 0-16
    float4 PositionAndRadius;
    // 16-20
    uint MaterialIndex;
    // Padding
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
    // 0-4
    int MaterialIndex;
};

struct SMesh
{
    // 0-8
    uint BoundingBoxIndex;
};

struct SHitInfo
{
    float2 Barycentrics;
    float Dist;
};

#endif

