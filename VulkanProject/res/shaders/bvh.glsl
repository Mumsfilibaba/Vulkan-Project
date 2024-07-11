#ifndef BVH_GLSL
#define BVH_GLSL
#include "math.glsl"

#define OBJECT_TYPE_UNKNOWN 0
#define OBJECT_TYPE_SPHERE 1
#define OBJECT_TYPE_QUAD 2
#define OBJECT_TYPE_MESH 3

#define BVH_ROOT_NODE_INDEX 0
#define BVH_MAX_DEPTH 32

struct FBoundingBox
{
    // 0-16
    vec4 BoxMinAndIndex;
    // 16-32
    vec4 BoxMaxAndNumTriangles;
};

float IntersectRayAABB(in vec3 BoxMin, in vec3 BoxMax, in FRay Ray)
{
    vec3 MinT = (BoxMin - Ray.Origin) * Ray.InvDirection;
    vec3 MaxT = (BoxMax - Ray.Origin) * Ray.InvDirection;

    vec3 T1 = min(MinT, MaxT);
    vec3 T2 = max(MinT, MaxT);

    float DistFar  = min(min(T2.x, T2.y), T2.z);
    float DistNear = max(max(T1.x, T1.y), T1.z);

    const bool bDidHit = DistFar >= DistNear && DistFar >= 0.0;
    return bDidHit ? DistNear : LARGE_NUMBER;
}

#endif