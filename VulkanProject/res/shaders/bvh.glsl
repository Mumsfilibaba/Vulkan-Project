#ifndef BVH_GLSL
#define BVH_GLSL

#define OBJECT_TYPE_UNKNOWN 0
#define OBJECT_TYPE_SPHERE 1
#define OBJECT_TYPE_QUAD 2
#define OBJECT_TYPE_MESH 3

#define BVH_ROOT_NODE_INDEX 0

struct FBvhNode
{
    // 0-16
    vec4 AABBMin;
    // 16-32
    vec4 AABBMax;
    // 32-44
    uint ChildIndex;
    uint FirstTriangleIndex;
    uint NumTriangles;

    // Padding
    uint Padding0;
};

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