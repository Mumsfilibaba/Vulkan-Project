#ifndef RAY_HLSLI
#define RAY_HLSLI

struct SRay
{
    float3 Origin;
    float3 Direction;
    float3 InvDirection;
};

struct SRayPayLoad
{
    float3 Normal;
    float3 Tangent;
    float3 Position;
    float3 BaryCentrics;
    float2 TexCoords;
    float  T;
    float  MinT;
    float  MaxT;
    uint   MaterialIndex;
    uint   bFrontFace;
    uint   bFromInside;
};

#endif
