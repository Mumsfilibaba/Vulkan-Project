#ifndef RAY_GLSL
#define RAY_GLSL

struct FRay
{
    vec3 Origin;
    vec3 Direction;
};

struct FRayPayLoad
{
    vec3  Normal;
    vec3  Position;
    float T;
    float MinT;
    float MaxT;
    uint  MaterialIndex;
    bool  bFrontFace;
    bool  bFromInside;
};

#endif