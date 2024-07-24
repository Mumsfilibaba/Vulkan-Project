#pragma once
#include "Core.h"
#include "ScenePrimitives.h"
#include "Model.h"

struct FBvhBuilder;

struct FShaderBoundingBox
{
    // 0-16
    glm::vec3 BoxMin;
    
    // Depending on the context this is etiher first child index or frist triangle index
    uint32_t PrimitiveIndex = 0;
    
    // 16-32
    glm::vec3 BoxMax;
    uint32_t NumTriangles = 0;
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

struct FBvhBoundingBox
{
    FBvhBoundingBox()
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

struct FBvhTriangle
{
    glm::vec3 Center;
    glm::vec3 BoundsMin;
    glm::vec3 BoundsMax;

    glm::vec3 Positions[3];
    uint32_t  Indicies[3];
    uint32_t  MaterialIndex;
};

struct FBvhBuilder
{
    FBvhBuilder(uint32_t InMaxDepth);
    
    void BuildHierarchy();
    void Finalize();
    void RecalculateBounds(size_t VolumeIndex);
    float EvaluateCost(size_t VolumeIndex, size_t AxisIndex, float SplitPos);
    
    std::vector<FBvhTriangle>    Triangles;
    std::vector<FBvhBoundingBox> BoundingBoxes;
    const uint32_t               MaxDepth;
    uint32_t                     Depth;
};

struct FBvhAccelerationStructure
{
    FBvhAccelerationStructure();
    
    void Build(const FMesh& Mesh, uint32_t MaxDepth);

    std::vector<FShaderTriangle>    m_Triangles;
    std::vector<FShaderBoundingBox> m_BoundingBoxes;
    
    struct
    {
        uint32_t MaxTrianglesInLeafNode;
        uint32_t Depth;
    } Stats;
};
