#ifndef RAY_HLSLI
#define RAY_HLSLI

struct SRay
{
    float3 Origin;
    float3 Direction;
    float3 InvDirection;
};

struct SRayPayload
{
    float3 Normal;
    float3 Tangent;
    float3 Position;
    float3 Barycentrics;
    float2 TexCoords;
    float  T;
    float  MinT;
    float  MaxT;
    uint   MaterialIndex;
    uint   FrontFace;
    uint   FromInside;
};

#endif

