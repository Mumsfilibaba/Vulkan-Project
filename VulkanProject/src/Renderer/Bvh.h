#pragma once
#include "Core.h"
#include "ScenePrimitives.h"
#include "Model.h"

struct FBoundingBoxBuilder;

struct FShaderBoundingBox
{
    // 0-16
    glm::vec3 BoxMin;
    uint32_t  TriangleOrChildIndex = 0;
    // 16-32
    glm::vec3 BoxMax;
    uint32_t  NumTriangles = 0;
};

struct FBoundingBox
{
    FBoundingBox()
        : BoxMin(std::numeric_limits<float>::max())
        , BoxMax(std::numeric_limits<float>::lowest())
        , Triangles()
        , FirstTriangleIndex(0)
        , NumTriangles(0)
        , ChildIndex(0)
    {
    }
    
    glm::vec3             BoxMin;
    glm::vec3             BoxMax;
    std::vector<uint32_t> Triangles;
    uint32_t              FirstTriangleIndex;
    uint32_t              NumTriangles;
    uint32_t              ChildIndex;
};

struct FBoundingBoxBuilder
{
    FBoundingBoxBuilder(uint32_t InMaxDepth);
    
    void BuildHierarchy();
    void Finalize();
    void RecalculateBounds(FBoundingBox& BoundingBox);
    
    std::vector<FTriangle>                 Triangles;
    std::vector<FBoundingBox>              BoundingBoxes;
    const uint32_t                         MaxDepth;
    uint32_t                               Depth;
    std::vector<std::pair<size_t, size_t>> DepthDebugIndicies;
};

struct FAccelerationStructure
{
    FAccelerationStructure();
    
    void Build(const FMesh& Mesh, uint32_t MaxDepth);

    std::vector<FShaderTriangle>           m_Triangles;
    std::vector<FShaderBoundingBox>        m_BoundingBoxes;
    std::vector<std::pair<size_t, size_t>> m_DepthIndicies;
    
    struct
    {
        uint32_t MaxTrianglesInLeafNode;
        uint32_t Depth;
    } Stats;
};
