#include "Bvh.h"
#include "SoftwareScene.h"
#include "Model.h"
#include <queue>

#define SAH_PER_TRIANGLE 0
#define SAH_OPTIMIZED 0
#define SAH_BINNED 1
#define SAH_NUM_SPLITS 512
#define SAH_NUM_BINS SAH_NUM_SPLITS

struct SBin
{
    SBin()
        : AABB()
        , TriangleCount(0)
    {
    }

    SAABB    AABB;
    uint32_t TriangleCount;
};

SBvhBuilder::SBvhBuilder(uint32_t InMaxDepth)
    : BoundingBoxes()
    , MaxDepth(InMaxDepth)
    , Depth(0)
{
}

void SBvhBuilder::BuildHierarchy()
{
    // Split bounding box along the longest axis
    std::queue<uint32_t> Queue;
    Queue.push(0);

    uint32_t CurrentDepth = 0;
    while (CurrentDepth < MaxDepth && !Queue.empty())
    {
        // Go through all the nodes
        size_t NumNodes = Queue.size();
        for (size_t i = 0; i < NumNodes; i++)
        {
            // Get first index to process
            const size_t CurrentIndex = Queue.front();
            Queue.pop();

            if (BoundingBoxes[CurrentIndex].Triangles.size() <= 2)
            {
                continue;
            }

            // Go through each axis and find the best cost
            float   BestSplit = 0.0f;
            int32_t BestAxis  = -1;
            float   BestCost  = std::numeric_limits<float>::max();

            const std::vector<uint32_t>& TriangleIndices = BoundingBoxes[CurrentIndex].Triangles;
            for (size_t Axis = 0; Axis < 3; Axis++)
            {
            #if SAH_PER_TRIANGLE
                for (uint32_t TriangleIndex : TriangleIndices)
                {
                    const float Cost = EvaluateCost(CurrentIndex, Axis, Triangles[TriangleIndex].Center[Axis]);
                    if (Cost < BestCost)
                    {
                        BestAxis  = Axis;
                        BestSplit = Triangles[TriangleIndex].Center[Axis];
                        BestCost  = Cost;
                    }
                }
            #elif SAH_OPTIMIZED
                float BoxMin = std::numeric_limits<float>::max();
                float BoxMax = std::numeric_limits<float>::lowest();

                for (uint32_t TriangleIndex : TriangleIndices)
                {
                    const float Center = Triangles[TriangleIndex].Center[Axis];
                    BoxMin = std::min(BoxMin, Center);
                    BoxMax = std::max(BoxMax, Center);
                }

                if (BoxMin == BoxMax)
                {
                    continue;
                }

                const float Extent = BoxMax - BoxMin;
                const float Scale  = Extent / static_cast<float>(SAH_NUM_SPLITS);
                for (size_t i = 0; i < SAH_NUM_SPLITS; i++)
                {
                    const float SplitPos = BoxMin + (static_cast<float>(i) * Scale);
                    const float Cost = EvaluateCost(CurrentIndex, Axis, SplitPos);
                    if (Cost < BestCost)
                    {
                        BestAxis  = Axis;
                        BestSplit = SplitPos;
                        BestCost  = Cost;
                    }
                }
            #elif SAH_BINNED
                float BoxMin = std::numeric_limits<float>::max();
                float BoxMax = std::numeric_limits<float>::lowest();

                for (uint32_t TriangleIndex : TriangleIndices)
                {
                    const float Center = Triangles[TriangleIndex].Center[Axis];
                    BoxMin = std::min(BoxMin, Center);
                    BoxMax = std::max(BoxMax, Center);
                }

                if (BoxMin == BoxMax)
                {
                    continue;
                }

                SBin Bins[SAH_NUM_BINS];
                float Scale = SAH_NUM_BINS / (BoxMax - BoxMin);
                for (uint32_t TriangleIndex : TriangleIndices)
                {
                    SBvhTriangle& Triangle = Triangles[TriangleIndex];

                    const int32_t BinIndex = std::max(0, std::min(SAH_NUM_BINS - 1, static_cast<int32_t>(((Triangle.Center[Axis] - BoxMin) * Scale))));
                    Bins[BinIndex].TriangleCount++;
                    for (size_t i = 0; i < 3; i++)
                    {
                        Bins[BinIndex].AABB.FitAroundPoint(Triangle.Positions[i]);
                    }
                }

                struct SBinInfo
                {
                    float   LeftArea   = 0.0f;
                    float   RightArea  = 0.0f;
                    int32_t LeftCount  = 0;
                    int32_t RightCount = 0;
                };

                int32_t LeftSum  = 0;
                int32_t RightSum = 0;

                SAABB    LeftBox;
                SAABB    RightBox;
                SBinInfo BinInfos[SAH_NUM_BINS - 1];
                for (int32_t i = 0; i < SAH_NUM_BINS - 1; i++)
                {
                    LeftSum += Bins[i].TriangleCount;

                    LeftBox.Grow(Bins[i].AABB);
                    BinInfos[i].LeftArea  = LeftBox.GetArea();
                    BinInfos[i].LeftCount = LeftSum;

                    RightSum += Bins[SAH_NUM_BINS - 1 - i].TriangleCount;

                    RightBox.Grow(Bins[SAH_NUM_BINS - 1 - i].AABB);
                    BinInfos[SAH_NUM_BINS - 2 - i].RightArea  = RightBox.GetArea();
                    BinInfos[SAH_NUM_BINS - 2 - i].RightCount = RightSum;
                }

                Scale = (BoxMax - BoxMin) / SAH_NUM_BINS;
                for (int32_t i = 0; i < SAH_NUM_BINS - 1; i++)
                {
                    const float PlaneCost = (BinInfos[i].LeftCount * BinInfos[i].LeftArea) + (BinInfos[i].RightCount * BinInfos[i].RightArea);
                    if (PlaneCost < BestCost)
                    {
                        BestAxis  = Axis;
                        BestSplit = BoxMin + Scale * (i + 1);
                        BestCost  = PlaneCost;
                    }
                }
            #endif
            }

            // Axis must be valid
            assert(BestAxis >= 0);

            // Add triangles to the child-nodes
            std::vector<uint32_t> LeftIndicies;
            std::vector<uint32_t> RightIndicies;
            for (uint32_t TriangleIndex : TriangleIndices)
            {
                // Add to either the right- or left- child
                SBvhTriangle& Triangle = Triangles[TriangleIndex];
                if (Triangle.Center[BestAxis] < BestSplit)
                {
                    LeftIndicies.emplace_back(TriangleIndex);
                }
                else
                {
                    RightIndicies.emplace_back(TriangleIndex);
                }
            }

            // Ensure all the triangles are assigned a single time
            assert(TriangleIndices.size() == LeftIndicies.size() + RightIndicies.size());

            // Abort if one child gets zero triangles
            if (LeftIndicies.size() == 0 || RightIndicies.size() == 0)
            {
                continue;
            }

            // Delete the old triangles
            BoundingBoxes[CurrentIndex].Triangles.clear();

            // Generate the new child-index
            const uint32_t NewIndex = BoundingBoxes.size();
            BoundingBoxes[CurrentIndex].ChildIndex = NewIndex;
            Queue.push(NewIndex);
            Queue.push(NewIndex + 1);

            // Create the new nodes
            BoundingBoxes.emplace_back();
            BoundingBoxes.emplace_back();

            // Assign the triangle indices
            BoundingBoxes[NewIndex].Triangles = std::move(LeftIndicies);
            BoundingBoxes[NewIndex + 1].Triangles = std::move(RightIndicies);

            // Grow the new boxes to ensure that all the triangles fully fit inside the boxes
            RecalculateBounds(NewIndex);
            RecalculateBounds(NewIndex + 1);
        }

        // Move to next depth
        CurrentDepth++;
    }

    // Save the depth for stats
    Depth = CurrentDepth;
}

void SBvhBuilder::Finalize()
{
    std::vector<SBvhTriangle> NewTriangles;
    NewTriangles.reserve(Triangles.size());

    for (SBvhBoundingBox& Box : BoundingBoxes)
    {
        // Only process leaf-nodes
        if (Box.ChildIndex != 0)
        {
            continue;
        }

        // Avoid boxes without triangles
        const size_t NumTriangles = Box.Triangles.size();
        if (!NumTriangles)
        {
            continue;
        }

        // Setup triangle information
        Box.NumTriangles       = NumTriangles;
        Box.FirstTriangleIndex = NewTriangles.size();

        // Insert triangles into the new array in the new order
        for (uint32_t TriangleIndex : Box.Triangles)
        {
            NewTriangles.push_back(Triangles[TriangleIndex]);
        }
    }

    // Replace the old triangles with the new ones
    Triangles = std::move(NewTriangles);
}

void SBvhBuilder::RecalculateBounds(size_t VolumeIndex)
{
    glm::vec3 BoxMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 BoxMax = glm::vec3(std::numeric_limits<float>::lowest());

    SBvhBoundingBox& BoundingBox = BoundingBoxes[VolumeIndex];
    for (uint32_t TriangleIndex : BoundingBox.Triangles)
    {
        SBvhTriangle& Triangle = Triangles[TriangleIndex];
        BoxMin = glm::min(BoxMin, Triangle.BoundsMin);
        BoxMin = glm::min(BoxMin, Triangle.BoundsMax);
        BoxMax = glm::max(BoxMax, Triangle.BoundsMin);
        BoxMax = glm::max(BoxMax, Triangle.BoundsMax);
    }

    BoundingBox.BoxMin = BoxMin;
    BoundingBox.BoxMax = BoxMax;
}

float SBvhBuilder::EvaluateCost(size_t VolumeIndex, size_t AxisIndex, float SplitPos)
{
    SAABB LeftBox;
    SAABB RightBox;
    size_t LeftCount  = 0;
    size_t RightCount = 0;

    SBvhBoundingBox& BoundingBox = BoundingBoxes[VolumeIndex];
    for (uint32_t TriangleIndex : BoundingBox.Triangles)
    {
        SBvhTriangle& Triangle = Triangles[TriangleIndex];
        if (Triangle.Center[AxisIndex] < SplitPos)
        {
            LeftBox.FitAroundPoint(Triangle.BoundsMin);
            LeftBox.FitAroundPoint(Triangle.BoundsMax);
            LeftCount++;
        }
        else
        {
            RightBox.FitAroundPoint(Triangle.BoundsMin);
            RightBox.FitAroundPoint(Triangle.BoundsMax);
            RightCount++;
        }
    }

    const float Cost = LeftCount * LeftBox.GetArea() + RightCount * RightBox.GetArea();
    return Cost > 0 ? Cost : std::numeric_limits<float>::max();
}

SBvhAccelerationStructure::SBvhAccelerationStructure()
    : m_TriangleInfo()
    , m_BoundingBoxes()
{
}

void SBvhAccelerationStructure::Build(const SModel& Model, uint32_t MaxDepth)
{
    SBvhBuilder BoundingBoxBuilder(MaxDepth);

    // Create all triangles
    for (size_t i = 0; i < Model.SubMeshes.size(); i++)
    {
        const SModel::SSubMesh& SubMesh = Model.SubMeshes[i];
        for (size_t j = 0; j < SubMesh.IndexCount; j += 3)
        {
            const size_t BaseIndex = SubMesh.IndexOffset + j;

            // Create a new triangle
            SBvhTriangle& Triangle = BoundingBoxBuilder.Triangles.emplace_back();
            Triangle.Indicies[0]  = Model.Indicies[BaseIndex + 0];
            Triangle.Indicies[1]  = Model.Indicies[BaseIndex + 1];
            Triangle.Indicies[2]  = Model.Indicies[BaseIndex + 2];
            Triangle.Positions[0] = Model.Vertices[Triangle.Indicies[0]].Position;
            Triangle.Positions[1] = Model.Vertices[Triangle.Indicies[1]].Position;
            Triangle.Positions[2] = Model.Vertices[Triangle.Indicies[2]].Position;

            // Calculate center of the triangle
            Triangle.Center = (Triangle.Positions[0] + Triangle.Positions[1] + Triangle.Positions[2]) / 3.0f;

            // Cache the bounds of the triangle
            Triangle.BoundsMin = glm::vec3(std::numeric_limits<float>::max());
            Triangle.BoundsMax = glm::vec3(std::numeric_limits<float>::lowest());

            for (size_t PosIdx = 0; PosIdx < 3; PosIdx++)
            {
                Triangle.BoundsMin = glm::min(Triangle.BoundsMin, Triangle.Positions[PosIdx]);
                Triangle.BoundsMax = glm::max(Triangle.BoundsMax, Triangle.Positions[PosIdx]);
            }

            // Set the MaterialIndex for this triangle
            Triangle.MaterialIndex = SubMesh.MaterialIndex;
        }
    }

    const size_t NumTriangles = Model.IndexCount / 3;
    assert((Model.IndexCount % 3) == 0);
    assert(NumTriangles == BoundingBoxBuilder.Triangles.size());

    // Create root-node and insert all triangles into it
    BoundingBoxBuilder.BoundingBoxes.emplace_back();
    for (size_t i = 0; i < BoundingBoxBuilder.Triangles.size(); i++)
    {
        BoundingBoxBuilder.BoundingBoxes[0].Triangles.push_back(i);
    }

    BoundingBoxBuilder.RecalculateBounds(0);

    // Subdivide and build all the bounding boxes
    BoundingBoxBuilder.BuildHierarchy();
    BoundingBoxBuilder.Finalize();

    // Convert triangles into shader-compatible structure
    m_TriangleInfo.reserve(BoundingBoxBuilder.Triangles.size());
    m_Indicies.reserve(BoundingBoxBuilder.Triangles.size() * 3);
    for (const SBvhTriangle& Triangle : BoundingBoxBuilder.Triangles)
    {
        STriangleInfoHLSL& TriangleInfo = m_TriangleInfo.emplace_back();
        TriangleInfo.MaterialIndex = Triangle.MaterialIndex;

        for (size_t i = 0; i < 3; i++)
        {
            m_Indicies.push_back(Triangle.Indicies[i]);
        }
    }

    m_TriangleInfo.shrink_to_fit();
    m_Indicies.shrink_to_fit();

    // Convert bounding-boxes into shader-compatible structure
    uint32_t MaxTriangleCount = 0;
    m_BoundingBoxes.reserve(BoundingBoxBuilder.BoundingBoxes.size());
    for (const SBvhBoundingBox& Box : BoundingBoxBuilder.BoundingBoxes)
    {
        if (Box.ChildIndex != 0)
        {
            assert(Box.FirstTriangleIndex == 0);
            assert(Box.NumTriangles == 0);
        }

        SShaderBoundingBox& ShaderBox = m_BoundingBoxes.emplace_back();
        ShaderBox.BoxMin         = Box.BoxMin;
        ShaderBox.BoxMax         = Box.BoxMax;
        ShaderBox.PrimitiveIndex = (Box.NumTriangles == 0) ? Box.ChildIndex : Box.FirstTriangleIndex;
        ShaderBox.NumTriangles   = Box.NumTriangles;

        if (ShaderBox.NumTriangles > 0)
        {
            const uint32_t LastTriangleIndex = ShaderBox.PrimitiveIndex + ShaderBox.NumTriangles;
            assert(LastTriangleIndex <= m_TriangleInfo.size());
            MaxTriangleCount = std::max(static_cast<uint32_t>(ShaderBox.NumTriangles), MaxTriangleCount);
        }
    }

    // Setup the stats
    Stats.Depth                  = BoundingBoxBuilder.Depth;
    Stats.MaxTrianglesInLeafNode = MaxTriangleCount;
}
