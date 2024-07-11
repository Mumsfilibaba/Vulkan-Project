#pragma once
#include "Core.h"
#include "ScenePrimitives.h"
#include "Model.h"

struct FBoundingBoxBuilder;

struct FShaderBoundingBox
{
    // 0-16
    glm::vec3 BoxMin;
    float TriangleOrChildIndex = 0.0f;
    // 16-32
    glm::vec3 BoxMax;
    float NumTriangles = 0.0f;
};

struct FAABB
{
    FAABB()
        : Min(std::numeric_limits<float>::max())
        , Max(std::numeric_limits<float>::lowest())
    {
    }
    
    void FitAroundPoint(const glm::vec3& Point)
    {
        Min = glm::min(Min, Point);
        Max = glm::max(Max, Point);
    }
    
    float GetArea() const
    {
        const glm::vec3 Extent = Max - Min;
        return Extent.x * Extent.y + Extent.y * Extent.z + Extent.z * Extent.x;
    }
    
    glm::vec3 Min;
    glm::vec3 Max;
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
    void RecalculateBounds(size_t VolumeIndex);
    float EvaluateCost(size_t VolumeIndex, size_t AxisIndex, float SplitPos);
    
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
