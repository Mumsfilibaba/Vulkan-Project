#include "Bvh.h"
#include "Scene.h"
#include "Model.h"
#include <queue>

FBoundingBoxBuilder::FBoundingBoxBuilder(uint32_t InMaxDepth)
    : BoundingBoxes()
    , MaxDepth(InMaxDepth)
    , Depth(0)
{
    // Allocate root
    BoundingBoxes.emplace_back();
}

void FBoundingBoxBuilder::InsertTriangle(const FTriangle& Triangle)
{
    // Grow bounding box to include this triangle
    FBoundingBox& Root = GetRoot();
    for (int32_t i = 0; i < 3; i++)
    {
        Root.GrowAroundPoint(Triangle.Positions[i]);
    }
    
    // Add the triangle
    Root.Triangles.push_back(Triangles.size());
    Triangles.push_back(Triangle);
}

void FBoundingBoxBuilder::BuildHierarchy()
{
    // Split bounding box along the longest axix
    std::queue<uint32_t> Queue;
    Queue.push(0);
    
    uint32_t CurrentDepth = 0;
    while (CurrentDepth < MaxDepth && !Queue.empty())
    {
        size_t NumNodes = Queue.size();
        for (size_t i = 0; i < NumNodes; i++)
        {
            // Get first index to process
            const size_t CurrentIndex = Queue.front();
            Queue.pop();

            if (BoundingBoxes[CurrentIndex].Triangles.size() <= 1)
            {
                continue;
            }
            
            const glm::vec3 ParentMin   = BoundingBoxes[CurrentIndex].BoxMin;
            const glm::vec3 ParentMax   = BoundingBoxes[CurrentIndex].BoxMax;
            const glm::vec3 Lengths     = ParentMax - ParentMin;
            const uint32_t  LongestAxis = (Lengths.x > Lengths.y) ? ((Lengths.x > Lengths.z) ? 0 : 2) : ((Lengths.y > Lengths.z) ? 1 : 2);
            const float     NewLength   = Lengths[LongestAxis] / 2.0f;
            
            // Generate the new child-index
            const uint32_t NewIndex = BoundingBoxes.size();
            BoundingBoxes[CurrentIndex].ChildIndex = NewIndex;
            Queue.push(NewIndex);
            Queue.push(NewIndex + 1);
                        
            // Create the new nodes
            BoundingBoxes.emplace_back(ParentMin, ParentMax);
            BoundingBoxes.emplace_back(ParentMin, ParentMax);
            
            FBoundingBox& LeftChild  = BoundingBoxes[NewIndex];
            FBoundingBox& RightChild = BoundingBoxes[NewIndex + 1];
            FBoundingBox& Parent     = BoundingBoxes[CurrentIndex];

            // Modify boxes
            LeftChild.BoxMax[LongestAxis] -= NewLength;
            RightChild.BoxMin[LongestAxis] += NewLength;
            
            // Add triangles to the child-nodes
            std::vector<uint32_t> TriangleIndices = std::move(Parent.Triangles);
            for (uint32_t TriangleIndex : TriangleIndices)
            {
                // Add to either the right- or left- child
                FTriangle& Triangle = Triangles[TriangleIndex];
                if (RightChild.Contains(Triangle.Center))
                {
                    RightChild.Triangles.emplace_back(TriangleIndex);
                }
                else
                {
                    LeftChild.Triangles.emplace_back(TriangleIndex);
                }
            }
            
            // Grow the new boxes to ensure that all the triangles fully fit inside the boxes
            for (uint32_t TriangleIndex : LeftChild.Triangles)
            {
                FTriangle& Triangle = Triangles[TriangleIndex];
                for (uint32_t i = 0; i < 3; i++)
                {
                    LeftChild.GrowAroundPoint(Triangle.Positions[i]);
                }
            }
            
            for (uint32_t TriangleIndex : RightChild.Triangles)
            {
                FTriangle& Triangle = Triangles[TriangleIndex];
                for (uint32_t i = 0; i < 3; i++)
                {
                    RightChild.GrowAroundPoint(Triangle.Positions[i]);
                }
            }
        }
        
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

FAccelerationStructure::FAccelerationStructure()
    : m_Triangles()
    , m_BoundingBoxes()
{
}

void FAccelerationStructure::Build(const FMesh& Mesh, uint32_t MaxDepth)
{
    FBoundingBoxBuilder BoundingBoxBuilder(MaxDepth);

    for (uint32_t i = 0; i < Mesh.Indicies.size(); i += 3)
    {
        // Set each index for the triangle
        FTriangle Triangle;
        Triangle.Indicies[0] = Mesh.Indicies[i + 0];
        Triangle.Indicies[1] = Mesh.Indicies[i + 1];
        Triangle.Indicies[2] = Mesh.Indicies[i + 2];
        
        Triangle.Positions[0] = Mesh.Positions[Triangle.Indicies[0]].Position;
        Triangle.Positions[1] = Mesh.Positions[Triangle.Indicies[1]].Position;
        Triangle.Positions[2] = Mesh.Positions[Triangle.Indicies[2]].Position;
        
        // Calculate center of the triangle
        glm::vec3 HalfPos = (Triangle.Positions[0] + Triangle.Positions[1]) / 2.0f;
        Triangle.Center = (HalfPos + Triangle.Positions[2]) / 2.0f;
        
        // Insert the triangle into the builder
        BoundingBoxBuilder.InsertTriangle(Triangle);
    }
    
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
            MaxTriangleCount = std::max(ShaderBox.NumTriangles, MaxTriangleCount);
        }
    }
    
    // Setup the stats
    Stats.Depth                  = BoundingBoxBuilder.Depth;
    Stats.MaxTrianglesInLeafNode = MaxTriangleCount;
}
