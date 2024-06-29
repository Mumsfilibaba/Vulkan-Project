#pragma once
#include "Core.h"
#include "ScenePrimitives.h"

struct FScene;

struct FBvhNode
{
    // 0-16
    glm::vec4 AABBMin;
    // 16-32
    glm::vec4 AABBMax;
    // 32-40
    uint32_t  ObjectType;
    uint32_t  ObjectIndex;

    // Padding
    uint Padding0;
    uint Padding1;
};

struct FBvhScene
{
    FBvhScene();
    
    void Build(const FScene& Scene);

    std::vector<FBvhNode> m_Nodes;
};
