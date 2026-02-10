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
#define MAX_MATERIALS 1024
#define MAX_BVH_NODES 300000
#define MAX_TRIANGLEMESHES 3

#define BACKGROUND_TYPE_NONE 0
#define BACKGROUND_TYPE_GRADIENT 1
#define BACKGROUND_TYPE_SKYBOX 2

class CBuffer;
class CSampler;

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

struct SSoftwareSceneSettings
{
    ESoftwareViewMode ViewMode;
    uint32_t          BackgroundType;
    float             GradientLightStrength;
    float             Exposure;
    uint32_t          NumBounces;
    float             FieldOfView;
    float             CameraSpeed;
};

struct SSoftwareScene : public IScene
{
    SSoftwareScene();
    virtual ~SSoftwareScene();

    // IScene Interface
    virtual void Initialize() override { }
    virtual void Reset() override { }

    virtual void OnRenderUI() override { }

    virtual float GetCameraSpeed() const override { return m_Settings.CameraSpeed; }
    virtual float GetFieldOfView() const override { return m_Settings.FieldOfView; }
    virtual float GetExposure() const override { return m_Settings.Exposure; }

    virtual CCamera& GetCamera() override { return m_Camera; }
    virtual const CCamera& GetCamera() const override { return m_Camera; }

    CCamera                m_Camera;
    SSoftwareSceneSettings m_Settings;

    // Materials
    std::vector<SMaterial>     m_Materials;
    std::vector<SMaterialGLSL> m_GpuMaterials;

    // Other primitive data
    std::vector<SSphereGLSL>   m_Spheres;
    std::vector<SQuadGLSL>     m_Quads;

    // BVH Container
    SBvhAccelerationStructure  m_AccelerationStructure;

    // Triangle Mesh Data
    std::vector<SVertexPosition>   m_VertexPositions;
    std::vector<SVertex>           m_Vertices;
    std::vector<uint32_t>          m_Indicies;
    std::vector<SMeshGLSL>         m_Meshes;
    std::vector<STriangleInfoGLSL> m_TriangleInfo;

    // CPU Buffers
    CBuffer*  m_pVertexPositionsBuffer;
    CBuffer*  m_pVertexBuffer;
    CBuffer*  m_pIndexBuffer;
    CBuffer*  m_pTriangleBuffer;
    CBuffer*  m_pBoundingBoxBuffer;
    bool      m_bUpdateBuffers;

    // Sampler for materials
    CSampler* m_pMaterialSampler;
    // Debugging
    CBuffer*  m_pAABBInstanceBuffer;
};

enum class EModelSceneType
{
    Default = 1,
    Sponza  = 2,
};

struct SModelScene : public SSoftwareScene
{
    SModelScene(EModelSceneType InType)
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

struct SSphereScene : public SSoftwareScene
{
    SSphereScene(ESphereSceneType InType)
        : Type(InType)
    {
    }
    
    virtual void Initialize() override;
    virtual void Reset() override;
    
    const ESphereSceneType Type;
};

struct SCornellBoxScene : public SSoftwareScene
{
    virtual void Initialize() override;
    virtual void Reset() override;
};
