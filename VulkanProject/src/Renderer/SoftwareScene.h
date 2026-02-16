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
#define MAX_TRIANGLEMESHES 1024
#define MAX_TLAS_NODES (MAX_TRIANGLEMESHES * 2)

#define BACKGROUND_TYPE_NONE 0
#define BACKGROUND_TYPE_GRADIENT 1
#define BACKGROUND_TYPE_SKYBOX 2

class CBuffer;
class CSampler;

enum class ESoftwareViewMode : uint32_t
{
    Render           = 0,
    Normals          = 1,
    GeometricNormals = 2,
    Tangents         = 3,
    Albedo           = 4,
    Barycentrics     = 5,
    TexCoords        = 6,
    BVHIntersection  = 7,
    Debug            = 8,
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
    struct SSceneModel
    {
        std::shared_ptr<SModel> Model;
        glm::vec3               Position;
        glm::vec3               Scale;
        glm::vec3               Rotation;
    };

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

    void AddModel(const std::shared_ptr<SModel>& Model, const glm::vec3& Position = glm::vec3(0.0f), const glm::vec3& Scale = glm::vec3(1.0f), const glm::vec3& Rotation = glm::vec3(0.0f))
    {
        m_ModelInstances.push_back({ Model, Position, Scale, Rotation });
    }

    CCamera                m_Camera;
    SSoftwareSceneSettings m_Settings;
    std::vector<SSceneModel> m_ModelInstances;

    // Materials
    std::vector<SMaterial>     m_Materials;
    std::vector<SMaterialHLSL> m_GpuMaterials;

    // Other primitive data
    std::vector<SSphereHLSL>   m_Spheres;
    std::vector<SQuadHLSL>     m_Quads;

    // BVH Container
    SBvhAccelerationStructure  m_AccelerationStructure;

    // Triangle Mesh Data
    std::vector<SVertexPosition>   m_VertexPositions;
    std::vector<SVertex>           m_Vertices;
    std::vector<uint32_t>          m_Indicies;
    std::vector<SMeshHLSL>         m_Meshes;
    std::vector<STriangleInfoHLSL> m_TriangleInfo;
    std::vector<SBoundingBoxHLSL> m_TLASBoundingBoxes;

    // CPU Buffers
    CBuffer*  m_pVertexPositionsBuffer;
    CBuffer*  m_pVertexBuffer;
    CBuffer*  m_pIndexBuffer;
    CBuffer*  m_pTriangleBuffer;
    CBuffer*  m_pBoundingBoxBuffer;
    CBuffer*  m_pTLASBoundingBoxBuffer;
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
