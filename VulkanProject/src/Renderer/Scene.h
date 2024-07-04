#pragma once
#include "Camera.h"
#include "Model.h"
#include "Bvh.h"
#include "ScenePrimitives.h"

#define MAX_QUADS 128
#define MAX_SPHERES 32
#define MAX_PLANES 32
#define MAX_TRIANGLES 300000
#define MAX_VERTICES (MAX_TRIANGLES * 3)
#define MAX_MATERIALS 32
#define MAX_BVH_NODES 4096
#define MAX_TRIANGLEMESHES 3

#define BACKGROUND_TYPE_NONE 0
#define BACKGROUND_TYPE_GRADIENT 1
#define BACKGROUND_TYPE_SKYBOX 2

class FBuffer;

enum class EViewMode : uint32_t
{
    Render = 0,
    Normals = 1,
    TopBVH = 2,
};

struct FSceneSettings
{
    EViewMode ViewMode;
    uint32_t  BackgroundType;
    float     Exposure;
    uint32_t  NumBounces;
    float     FieldOfView;
    float     CameraSpeed;
};

struct FScene
{
    FScene();
    ~FScene();

    virtual void Initialize() {}
    virtual void Reset() {}

    FCamera        m_Camera;
    FSceneSettings m_Settings;

    // Materials
    std::vector<FShaderMaterial> m_Materials;
    
    // Triangle Mesh Data
    std::vector<FVertexPosOnly>  m_Vertices;
    std::vector<FShaderTriangle> m_Triangles;
    std::vector<FShaderMesh>     m_Meshes;
    
    // Other primitive data
    std::vector<FShaderSphere>   m_Spheres;
    std::vector<FShaderQuad>     m_Quads;

    // Bvh Container
    FAccelerationStructure       m_AccelerationStructure;
    
    // CPU Buffers
    FBuffer* m_pTriangleBuffer;
    FBuffer* m_pBoundingBoxBuffer;
};

enum class EModelSceneType
{
    Default = 1,
    Sponza = 2,
};

struct FModelScene : public FScene
{
    FModelScene(EModelSceneType InType)
        : Type(InType)
    {
    }
    
    virtual void Initialize() override;
    virtual void Reset() override;
    
    const EModelSceneType Type;
};

enum class ESphereSceneType
{
    Default = 1,
    ColoredRoughGlass = 2,
    PolishedGlass = 3,
    RoughGlass = 4, 
};

struct FSphereScene : public FScene
{
    FSphereScene(ESphereSceneType InType)
        : Type(InType)
    {
    }
    
    virtual void Initialize() override;
    virtual void Reset() override;
    
    const ESphereSceneType Type;
};

struct FCornellBoxScene : public FScene
{
    virtual void Initialize() override;
    virtual void Reset() override;
};
