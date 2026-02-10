#ifndef RAY_GLSL
#define RAY_GLSL

struct SRay
{
    vec3 Origin;
    vec3 Direction;
    vec3 InvDirection;
};

struct SRayPayLoad
{
    vec3  Normal;
    vec3  Tangent;
    vec3  Position;
    vec3  BaryCentrics;
    vec2  TexCoords;
    float T;
    float MinT;
    float MaxT;
    uint  MaterialIndex;
    bool  bFrontFace;
    bool  bFromInside;
};

#endif