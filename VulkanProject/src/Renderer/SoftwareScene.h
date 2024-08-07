#pragma once
#include "Camera.h"
#include "Model.h"
#include "Bvh.h"
#include "ScenePrimitives.h"
#include "IScene.h"

#define MAX_QUADS 128
#define MAX_SPHERES 32
#define MAX_PLANES 32
#define MAX_TRIANGLES 300000
#define MAX_VERTICES (MAX_TRIANGLES * 3)
#define MAX_MATERIALS 32
#define MAX_BVH_NODES 300000
#define MAX_TRIANGLEMESHES 3

#define BACKGROUND_TYPE_NONE 0
#define BACKGROUND_TYPE_GRADIENT 1
#define BACKGROUND_TYPE_SKYBOX 2

class FBuffer;
class FSampler;

enum class ESoftwareViewMode : uint32_t
{
    Render          = 0,
    Normals         = 1,
    Albedo          = 2,
    Barycentrics    = 3,
    TexCoords       = 4,
    BVHIntersection = 5,
    Debug           = 6,
};

struct FSoftwareSceneSettings
{
    ESoftwareViewMode ViewMode;
    uint32_t          BackgroundType;
    float             GradientLightStrength;
    float             Exposure;
    uint32_t          NumBounces;
    float             FieldOfView;
    float             CameraSpeed;
};

struct FSoftwareScene : public IScene
{
    FSoftwareScene();
    virtual ~FSoftwareScene();

    // IScene Interface
    virtual void Initialize() override { }
    virtual void Reset() override { }

    virtual void OnRenderUI() override { }

    virtual float GetCameraSpeed() const override { return m_Settings.CameraSpeed; }
    virtual float GetFieldOfView() const override { return m_Settings.FieldOfView; }
    virtual float GetExposure() const override { return m_Settings.Exposure; }

    virtual FCamera& GetCamera() override { return m_Camera; }
    virtual const FCamera& GetCamera() const override { return m_Camera; }

    FCamera                m_Camera;
    FSoftwareSceneSettings m_Settings;

    // Materials
    std::vector<FMaterial>       m_Materials;
    std::vector<FShaderMaterial> m_GpuMaterials;
    
    // Triangle Mesh Data
    std::vector<FVertexPosOnly> m_Vertices;
    std::vector<FVertexEx>      m_VerticesEx;
    std::vector<uint32_t>       m_Indicies;
    std::vector<FShaderMesh>    m_Meshes;
    
    // Other primitive data
    std::vector<FShaderSphere>  m_Spheres;
    std::vector<FShaderQuad>    m_Quads;

    // BVH Container
    FBvhAccelerationStructure m_AccelerationStructure;
    
    // CPU Buffers
    FBuffer* m_pVertexBuffer;
    FBuffer* m_pVertexExBuffer;
    FBuffer* m_pTriangleBuffer;
    FBuffer* m_pBoundingBoxBuffer;
    bool     m_bUpdateBuffers;
    
    // Sampler for materials
    FSampler* m_pMaterialSampler;
    
    // Debugging
    FBuffer* m_pMeshVertexBuffer;
    FBuffer* m_pMeshIndexBuffer;
    FBuffer* m_pAABBInstanceBuffer;
};

enum class EModelSceneType
{
    Default = 1,
    Sponza  = 2,
};

struct FModelScene : public FSoftwareScene
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
    Default           = 1,
    ColoredRoughGlass = 2,
    PolishedGlass     = 3,
    RoughGlass        = 4,
};

struct FSphereScene : public FSoftwareScene
{
    FSphereScene(ESphereSceneType InType)
        : Type(InType)
    {
    }
    
    virtual void Initialize() override;
    virtual void Reset() override;
    
    const ESphereSceneType Type;
};

struct FCornellBoxScene : public FSoftwareScene
{
    virtual void Initialize() override;
    virtual void Reset() override;
};
