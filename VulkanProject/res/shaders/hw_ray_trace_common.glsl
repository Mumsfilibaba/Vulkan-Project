#ifndef HW_RAY_TRACE_COMMON_GLSL
#define HW_RAY_TRACE_COMMON_GLSL

struct FRayPayLoad
{
    vec3  HitNormal;
    vec3  HitTangent;
    vec3  HitPosition;
    vec3  HitBarycentrics;
    vec3  MissEmissive;
    vec2  HitTexCoord;
    uint  HitMaterialIndex;
    float HitT;
    bool  bFromInside;
};

struct FSceneSettings 
{
    // 0-16
    uint NumMaterials;
    uint BackgroundType;
    uint NumBounces;
    uint ViewMode;
    // 16-20
    float GradientLightStrength;

    // Padding
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

#endif