#pragma once
#include "Core.h"
#include "ScenePrimitives.h"
#include "Model.h"

struct SBvhBuilder;

struct SBoundingBoxHLSL
{
    // 0-16
    glm::vec3 BoxMin         = {};
    uint32_t  PrimitiveIndex = 0; // Depending on the context this is either first child index or first triangle index
    // 16-32
    glm::vec3 BoxMax         = {};
    uint32_t  NumTriangles   = 0;
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

struct SBvhTLASBuilder
{
    struct SPrimitive
    {
        uint32_t  MeshIndex = 0;
        glm::vec3 BoxMin    = glm::vec3(0.0f);
        glm::vec3 BoxMax    = glm::vec3(0.0f);
        glm::vec3 Center    = glm::vec3(0.0f);
    };

    SBvhTLASBuilder(const std::vector<SMeshHLSL>& InMeshes, const std::vector<SBoundingBoxHLSL>& InBLASNodes);

    void Build();

    std::vector<SBoundingBoxHLSL> BoundingBoxes;

private:
    SAABB TransformAABBToWorld(const glm::vec3& BoxMin, const glm::vec3& BoxMax, const glm::mat4& LocalToWorld) const;
    void BuildNodeRecursive(uint32_t NodeIndex, size_t Start, size_t End);

    std::vector<SPrimitive> m_Primitives;
};

struct SBvhAccelerationStructure
{
    struct SBLASInfo
    {
        uint32_t RootBoundingBoxIndex  = 0;
        uint32_t FirstBoundingBoxIndex = 0;
        uint32_t NumBoundingBoxes      = 0;
        uint32_t FirstTriangleIndex    = 0;
        uint32_t NumTriangles          = 0;
    };

    SBvhAccelerationStructure();

    void Clear();
    void Build(const SModel& Model, uint32_t MaxDepth);
    uint32_t AddBLAS(const SModel& Model, uint32_t MaxDepth, uint32_t VertexIndexOffset = 0, int32_t MaterialIndexOffset = 0);

    std::vector<SBLASInfo>          m_BLAS;
    std::vector<STriangleInfoHLSL>  m_TriangleInfo;
    std::vector<uint32_t>           m_Indicies;
    std::vector<SBoundingBoxHLSL> m_BoundingBoxes;

    struct
    {
        uint32_t MaxTrianglesInLeafNode = 0;
        uint32_t Depth                  = 0;
    } Stats;
};
