#ifndef HW_RAY_TRACE_COMMON_GLSL
#define HW_RAY_TRACE_COMMON_GLSL

struct FRayPayLoad
{
    vec3 HitNormal;
    vec3 HitPosition;
    vec3 HitAlbedo;
    vec3 HitEmissive;
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