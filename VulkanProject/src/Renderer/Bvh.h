#pragma once
#include "Core.h"
#include "ScenePrimitives.h"
#include "Model.h"

struct SBvhBuilder;

struct SShaderBoundingBox
{
    // 0-16
    glm::vec3 BoxMin;
    // Depending on the context this is either first child index or first triangle index
    uint32_t PrimitiveIndex = 0; 
    // 16-32
    glm::vec3 BoxMax;
    uint32_t  NumTriangles = 0;
};

struct SAABB
{
    SAABB()
        : Min(std::numeric_limits<float>::max())
        , Max(std::numeric_limits<float>::lowest())
    {
    }
    
    void Grow(const SAABB& AABB)
    {
        if (AABB.Min.x != std::numeric_limits<float>::max() && AABB.Min.y != std::numeric_limits<float>::max() && AABB.Min.z != std::numeric_limits<float>::max())
        {
            FitAroundPoint(AABB.Min);
        }

        if (AABB.Max.x != std::numeric_limits<float>::lowest() && AABB.Max.y != std::numeric_limits<float>::lowest() && AABB.Max.z != std::numeric_limits<float>::lowest())
        {
            FitAroundPoint(AABB.Max);
        }
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

struct SBvhBoundingBox
{
    SBvhBoundingBox()
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

struct SBvhTriangle
{
    glm::vec3 Center;
    glm::vec3 BoundsMin;
    glm::vec3 BoundsMax;

    glm::vec3 Positions[3];
    uint32_t  Indicies[3];
    uint32_t  MaterialIndex;
};

struct SBvhBuilder
{
    SBvhBuilder(uint32_t InMaxDepth);
    
    void BuildHierarchy();
    void Finalize();
    void RecalculateBounds(size_t VolumeIndex);
    float EvaluateCost(size_t VolumeIndex, size_t AxisIndex, float SplitPos);
    
    std::vector<SBvhTriangle>    Triangles;
    std::vector<SBvhBoundingBox> BoundingBoxes;
    const uint32_t               MaxDepth;
    uint32_t                     Depth;
};

struct SBvhAccelerationStructure
{
    SBvhAccelerationStructure();

    void Build(const SModel& Model, uint32_t MaxDepth);

    std::vector<STriangleInfoHLSL>  m_TriangleInfo;
    std::vector<uint32_t>           m_Indicies;
    std::vector<SShaderBoundingBox> m_BoundingBoxes;

    struct
    {
        uint32_t MaxTrianglesInLeafNode;
        uint32_t Depth;
    } Stats;
};
