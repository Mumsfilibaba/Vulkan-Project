#include "Bvh.h"
#include "Scene.h"
#include "Model.h"
#include <queue>

#define SAH_PER_TRIANGLE 0
#define SAH_OPTIMIZED 1
#define SAH_NUM_SPLITS 100

FBoundingBoxBuilder::FBoundingBoxBuilder(uint32_t InMaxDepth)
    : BoundingBoxes()
    , MaxDepth(InMaxDepth)
    , Depth(0)
{
}

void FBoundingBoxBuilder::BuildHierarchy()
{
    // Split bounding box along the longest axix
    std::queue<uint32_t> Queue;
    Queue.push(0);
    
    DepthDebugIndicies =
    {
        { 0, 1 }
    };
    
    uint32_t CurrentDepth = 0;
    while (CurrentDepth < MaxDepth && !Queue.empty())
    {
        // Store the start index for this depth
        std::pair<size_t, size_t>& DebugIndicies = DepthDebugIndicies.emplace_back();
        DebugIndicies.first = BoundingBoxes.size();
        
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
            
        #if SAH_OPTIMIZED
            const size_t NumSplits = SAH_NUM_SPLITS;
            const glm::vec3 Extent = BoundingBoxes[CurrentIndex].BoxMax - BoundingBoxes[CurrentIndex].BoxMin;
        #endif
            
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
                const float PerSplitDistance = Extent[Axis] / static_cast<float>(NumSplits);
                for (size_t i = 0; i < NumSplits; i++)
                {
                    const float SplitPos = BoundingBoxes[CurrentIndex].BoxMin[Axis] + (static_cast<float>(i) * PerSplitDistance);
                    const float Cost = EvaluateCost(CurrentIndex, Axis, SplitPos);
                    if (Cost < BestCost)
                    {
                        BestAxis  = Axis;
                        BestSplit = SplitPos;
                        BestCost  = Cost;
                    }
                }
            #endif
            }
            
            // Add triangles to the child-nodes
            std::vector<uint32_t> LeftIndicies;
            std::vector<uint32_t> RightIndicies;
            for (uint32_t TriangleIndex : TriangleIndices)
            {
                // Add to either the right- or left- child
                FTriangle& Triangle = Triangles[TriangleIndex];
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
        
        // Store the end index for this depth
        DebugIndicies.second = BoundingBoxes.size();
        
        // Move to next depth
        CurrentDepth++;
    }
    
    // Save the depth for stats
    Depth = CurrentDepth;
}

void FBoundingBoxBuilder::Finalize()
{
    std::vector<FTriangle> NewTriangles;
    NewTriangles.reserve(Triangles.size());
    
    for (FBoundingBox& Box : BoundingBoxes)
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
        
        // Insert trianfles into the new array in the new order
        for (uint32_t TriangleIndex : Box.Triangles)
        {
            NewTriangles.push_back(Triangles[TriangleIndex]);
        }
    }
    
    // Replace the old triangles with the new ones
    Triangles = std::move(NewTriangles);
}

void FBoundingBoxBuilder::RecalculateBounds(size_t VolumeIndex)
{
    glm::vec3 BoxMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 BoxMax = glm::vec3(std::numeric_limits<float>::lowest());
    
    FBoundingBox& BoundingBox = BoundingBoxes[VolumeIndex];
    for (uint32_t TriangleIndex : BoundingBox.Triangles)
    {
        FTriangle& Triangle = Triangles[TriangleIndex];
        BoxMin = glm::min(BoxMin, Triangle.Positions[0]);
        BoxMin = glm::min(BoxMin, Triangle.Positions[1]);
        BoxMin = glm::min(BoxMin, Triangle.Positions[2]);
        
        BoxMax = glm::max(BoxMax, Triangle.Positions[0]);
        BoxMax = glm::max(BoxMax, Triangle.Positions[1]);
        BoxMax = glm::max(BoxMax, Triangle.Positions[2]);
    }
    
    BoundingBox.BoxMin = BoxMin;
    BoundingBox.BoxMax = BoxMax;
}

float FBoundingBoxBuilder::EvaluateCost(size_t VolumeIndex, size_t AxisIndex, float SplitPos)
{
    FAABB LeftBox;
    FAABB RightBox;
    size_t LeftCount  = 0;
    size_t RightCount = 0;
    
    FBoundingBox& BoundingBox = BoundingBoxes[VolumeIndex];
    for (uint32_t TriangleIndex : BoundingBox.Triangles)
    {
        FTriangle& Triangle = Triangles[TriangleIndex];
        if (Triangle.Center[AxisIndex] < SplitPos)
        {
            LeftBox.FitAroundPoint(Triangle.Positions[0]);
            LeftBox.FitAroundPoint(Triangle.Positions[1]);
            LeftBox.FitAroundPoint(Triangle.Positions[2]);
            LeftCount++;
        }
        else
        {
            RightBox.FitAroundPoint(Triangle.Positions[0]);
            RightBox.FitAroundPoint(Triangle.Positions[1]);
            RightBox.FitAroundPoint(Triangle.Positions[2]);
            RightCount++;
        }
    }
    
    const float Cost = LeftCount * LeftBox.GetArea() + RightCount * RightBox.GetArea();
    return Cost > 0 ? Cost : std::numeric_limits<float>::max();
}

FAccelerationStructure::FAccelerationStructure()
    : m_Triangles()
    , m_BoundingBoxes()
{
}

void FAccelerationStructure::Build(const FMesh& Mesh, uint32_t MaxDepth)
{
    FBoundingBoxBuilder BoundingBoxBuilder(MaxDepth);
    
    // Create all triangles
    for (uint32_t i = 0; i < Mesh.Indicies.size(); i += 3)
    {
        // Set each index for the triangle
        FTriangle Triangle;
        Triangle.Indicies[0] = Mesh.Indicies[i + 0];
        Triangle.Indicies[1] = Mesh.Indicies[i + 1];
        Triangle.Indicies[2] = Mesh.Indicies[i + 2];
        
        Triangle.Positions[0] = Mesh.Vertices[Triangle.Indicies[0]].Position;
        Triangle.Positions[1] = Mesh.Vertices[Triangle.Indicies[1]].Position;
        Triangle.Positions[2] = Mesh.Vertices[Triangle.Indicies[2]].Position;
        
        // Calculate center of the triangle
        Triangle.Center = (Triangle.Positions[0] + Triangle.Positions[1] + Triangle.Positions[2]) / 3.0f;
        
        // Insert the triangle into the builder
        BoundingBoxBuilder.Triangles.push_back(Triangle);
    }
    
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
    m_Triangles.reserve(BoundingBoxBuilder.Triangles.size());
    for (const FTriangle& Triangle : BoundingBoxBuilder.Triangles)
    {
        FShaderTriangle& ShaderTriangle = m_Triangles.emplace_back();
        ShaderTriangle.Index0 = Triangle.Indicies[0];
        ShaderTriangle.Index1 = Triangle.Indicies[1];
        ShaderTriangle.Index2 = Triangle.Indicies[2];
    }

    // Convert bounding-boxes into shader-compatible structure
    uint32_t MaxTriangleCount = 0;
    m_BoundingBoxes.reserve(BoundingBoxBuilder.BoundingBoxes.size());
    for (const FBoundingBox& Box : BoundingBoxBuilder.BoundingBoxes)
    {
        if (Box.ChildIndex != 0)
        {
            assert(Box.FirstTriangleIndex == 0);
            assert(Box.NumTriangles == 0);
        }
        
        FShaderBoundingBox& ShaderBox = m_BoundingBoxes.emplace_back();
        ShaderBox.BoxMin               = Box.BoxMin;
        ShaderBox.BoxMax               = Box.BoxMax;
        ShaderBox.TriangleOrChildIndex = (Box.NumTriangles == 0) ? Box.ChildIndex : Box.FirstTriangleIndex;
        ShaderBox.NumTriangles         = Box.NumTriangles;

        if (ShaderBox.NumTriangles > 0)
        {
            const uint32_t LastTriangleIndex = ShaderBox.TriangleOrChildIndex + ShaderBox.NumTriangles;
            assert(LastTriangleIndex <= m_Triangles.size());
            MaxTriangleCount = std::max(static_cast<uint32_t>(ShaderBox.NumTriangles), MaxTriangleCount);
        }
    }
    
    // Setup the stats
    Stats.Depth = BoundingBoxBuilder.Depth;
    Stats.MaxTrianglesInLeafNode = MaxTriangleCount;
    
    // Store the depth-indices
    m_DepthIndicies = BoundingBoxBuilder.DepthDebugIndicies;
}
