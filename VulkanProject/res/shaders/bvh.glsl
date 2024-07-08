#ifndef BVH_GLSL
#define BVH_GLSL

#define OBJECT_TYPE_UNKNOWN 0
#define OBJECT_TYPE_SPHERE 1
#define OBJECT_TYPE_QUAD 2
#define OBJECT_TYPE_MESH 3

#define BVH_ROOT_NODE_INDEX 0
#define BVH_MAX_DEPTH 32

struct FBoundingBox
{
    // 0-16
    float MinAABB[3];
    uint TriangleOrChildIndex;
    // 16-32
    float MaxAABB[3];
    uint NumTriangles;
};

vec3 GetBoundingBoxMin(in FBoundingBox Node)
{
    return vec3(Node.MinAABB[0], Node.MinAABB[1], Node.MinAABB[2]);
}

vec3 GetBoundingBoxMax(in FBoundingBox Node)
{
    return vec3(Node.MaxAABB[0], Node.MaxAABB[1], Node.MaxAABB[2]);
}

vec2 IntersectRayAABB(in vec3 BoxMin, in vec3 BoxMax, in FRay Ray)
{
    vec3 MinT = (BoxMin - Ray.Origin) / Ray.Direction;
    vec3 MaxT = (BoxMax - Ray.Origin) / Ray.Direction;

    vec3 T1 = min(MinT, MaxT);
    vec3 T2 = max(MinT, MaxT);

    float NearT = max(max(T1.x, T1.y), T1.z);
    float FarT  = min(min(T2.x, T2.y), T2.z);
    return vec2(FarT, NearT);
}

#endif