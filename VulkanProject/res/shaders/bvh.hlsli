#ifndef BVH_HLSLI
#define BVH_HLSLI

#include "math.hlsli"

#define OBJECT_TYPE_UNKNOWN 0
#define OBJECT_TYPE_SPHERE 1
#define OBJECT_TYPE_QUAD 2
#define OBJECT_TYPE_MESH 3

#define BVH_ROOT_NODE_INDEX 0
#define BVH_MAX_DEPTH 32

struct SBoundingBox
{
    // 0-16
    float4 BoxMinAndIndex;
    // 16-32
    float4 BoxMaxAndNumTriangles;
};

float IntersectRayAABB(float3 boxMin, float3 boxMax, float3 rayOrigin, float3 invRayDirection)
{
    const float3 minT = (boxMin - rayOrigin) * invRayDirection;
    const float3 maxT = (boxMax - rayOrigin) * invRayDirection;

    const float3 t1 = min(minT, maxT);
    const float3 t2 = max(minT, maxT);

    const float distFar  = min(min(t2.x, t2.y), t2.z);
    const float distNear = max(max(t1.x, t1.y), t1.z);

    const bool didHit = distFar >= distNear && distFar >= 0.0;
    return didHit ? distNear : LARGE_NUMBER;
}

#endif
