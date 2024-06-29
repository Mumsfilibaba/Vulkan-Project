#ifndef BVH_GLSL
#define BVH_GLSL

#define OBJECT_TYPE_UNKNOWN 0
#define OBJECT_TYPE_SPHERE 1
#define OBJECT_TYPE_QUAD 2
#define OBJECT_TYPE_TRIANGLEMESH 3

struct FBvhNode
{
    // 0-16
    vec4 AABBMin;
    // 16-32
    vec4 AABBMax;
    // 32-40
    uint ObjectType;
    uint ObjectIndex;

    // Padding
    uint Padding0;
    uint Padding1;
};

#endif