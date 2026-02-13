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

float IntersectRayAABB(float3 BoxMin, float3 BoxMax, float3 RayOrigin, float3 InvRayDirection)
{
    const float3 MinT = (BoxMin - RayOrigin) * InvRayDirection;
    const float3 MaxT = (BoxMax - RayOrigin) * InvRayDirection;

    const float3 t1 = min(MinT, MaxT);
    const float3 t2 = max(MinT, MaxT);

    const float DistFar  = min(min(t2.x, t2.y), t2.z);
    const float DistNear = max(max(t1.x, t1.y), t1.z);

    const bool DidHit = DistFar >= DistNear && DistFar >= 0.0;
    return DidHit ? DistNear : LARGE_NUMBER;
}

#endif


