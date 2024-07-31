#ifndef HW_RAY_TRACE_COMMON_GLSL
#define HW_RAY_TRACE_COMMON_GLSL

struct FRayPayLoad
{
    vec3 HitNormal;
    vec3 HitPosition;
    vec3 HitAlbedo;
    vec3 HitEmissive;
};

#endif