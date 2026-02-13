#ifndef COMMON_STRUCTS_HLSLI
#define COMMON_STRUCTS_HLSLI

struct SCameraBuffer
{
    float4x4 Projection;
    float4x4 View;
    float4x4 InverseProjection;
    float4x4 InverseView;
    float4   Position;
    float4   Forward;
    float    FieldOfViewDegrees;
    // Padding
    uint     Padding0;
    uint     Padding1;
    uint     Padding2;
};

struct SRandomBuffer
{
    uint FrameIndex;
    uint HaltonIndex;
    // Padding
    uint Padding0;
    uint Padding1;
};

#ifndef COMMON_STRUCTS_NO_HW_RAYTRACE

struct SRayPayload
{
    float3 HitNormal;
    float3 HitTangent;
    float3 HitPosition;
    float3 HitBarycentrics;
    float3 MissEmissive;
    float2 HitTexCoord;
    uint   HitMaterialIndex;
    float  HitT;
    uint   FromInside;
};

struct SSceneSettings
{
    uint  NumMaterials;
    uint  BackgroundType;
    uint  NumBounces;
    uint  ViewMode;
    float GradientLightStrength;
    uint  Padding0;
    uint  Padding1;
    uint  Padding2;
};

#endif

#endif

