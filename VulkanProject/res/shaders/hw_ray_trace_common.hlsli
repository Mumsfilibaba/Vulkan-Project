#ifndef HW_RAY_TRACE_COMMON_HLSLI
#define HW_RAY_TRACE_COMMON_HLSLI

struct SRayPayLoad
{
    float3 HitNormal;
    float3 HitTangent;
    float3 HitPosition;
    float3 HitBarycentrics;
    float3 MissEmissive;
    float2 HitTexCoord;
    uint   HitMaterialIndex;
    float  HitT;
    uint   bFromInside;
};

struct SSceneSettings
{
    // 0-16
    uint  NumMaterials;
    uint  BackgroundType;
    uint  NumBounces;
    uint  ViewMode;
    // 16-20
    float GradientLightStrength;
    
    // Padding
    uint  Padding0;
    uint  Padding1;
    uint  Padding2;
};

#endif
