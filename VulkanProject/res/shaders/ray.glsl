#ifndef RAY_GLSL
#define RAY_GLSL

struct FRay
{
    vec3 Origin;
    vec3 Direction;
    vec3 InvDirection;
};

struct FRayPayLoad
{
    vec3  Normal;
    vec3  Position;
    vec3  BaryCentrics;
    float T;
    float MinT;
    float MaxT;
    uint  MaterialIndex;
    bool  bFrontFace;
    bool  bFromInside;
};

#endif