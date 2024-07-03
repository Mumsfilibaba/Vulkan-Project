#pragma once
#include "Core.h"
#include "ScenePrimitives.h"
#include "Model.h"

struct FBoundingBoxBuilder;

struct FShaderBoundingBox
{
    // 0-16
    glm::vec4 BoxMin;
    // 16-32
    glm::vec4 BoxMax;
    // 32-44
    uint32_t ChildIndex = 0;
    uint32_t FirstTriangleIndex = 0;
    uint32_t NumTriangles = 0;
    
    // Padding
    uint32_t Padding0;
};

struct FBoundingBox
{
    FBoundingBox()
        : BoxMin(std::numeric_limits<float>::max())
        , BoxMax(std::numeric_limits<float>::lowest())
        , Triangles()
        , ChildIndex(0)
    {
    }
    
    FBoundingBox(const glm::vec3& InBoxMin, const glm::vec3& InBoxMax)
        : BoxMin(InBoxMin)
        , BoxMax(InBoxMax)
        , Triangles()
        , FirstTriangleIndex(0)
        , NumTriangles(0)
        , ChildIndex(0)
    {
    }
    
    void GrowAroundPoint(const glm::vec3& Point)
    {
        BoxMin = glm::min(Point, BoxMin);
        BoxMax = glm::max(Point, BoxMax);
    }
    
    bool Contains(const glm::vec3& Point) const
    {
        return glm::all(glm::greaterThanEqual(Point, BoxMin)) && glm::all(glm::lessThan(Point, BoxMax));
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
    
    void InsertTriangle(const FTriangle& Triangle);
    void BuildHierarchy();
    void Finalize();
    
    FBoundingBox& GetRoot()
    {
        return BoundingBoxes[0];
    }
    
    std::vector<FTriangle>    Triangles;
    std::vector<FBoundingBox> BoundingBoxes;
    uint32_t                  MaxDepth;
};

struct FAccelerationStructure
{
    FAccelerationStructure();
    
    void Build(const FMesh& Mesh);

    std::vector<FShaderTriangle>    m_Triangles;
    std::vector<FShaderBoundingBox> m_BoundingBoxes;
};
