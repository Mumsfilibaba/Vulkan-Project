#pragma once
#include "Camera.h"
#include "Model.h"
#include "IScene.h"
#include "Vulkan/AccelerationStructure.h"

#define MAX_MATERIALS 1024
#define MAX_NUM_MESHES 1024

class CSampler;

struct SMeshInfo
{
    uint64_t VertexBufferAddress = 0;
    uint64_t IndexBufferAddress  = 0;
    uint64_t MaterialIndex       = 0;
};

enum class EViewMode : uint32_t
{
    Render       = 0,
    Normals      = 1,
    Albedo       = 2,
    Barycentrics = 3,
    TexCoords    = 4,
};

enum class EBackgroundType : uint32_t
{
    None     = 0,
    Gradient = 1, 
    Skybox   = 2,
};

enum class ESceneType
{
    Spheres = 0,
    CornellBox,
    Triangles,
    Sponza,
    PolishedGlassSpheres,
    RoughColoredGlassSpheres,
    RoughTransparentGlassSpheres,
};

struct SSceneSettings
{
    EViewMode       ViewMode;
    EBackgroundType BackgroundType;
    float           GradientLightStrength;
    float           Exposure;
    uint32_t        NumBounces;
    float           FieldOfView;
    float           CameraSpeed;
};


struct SScene : public IScene
{
    struct SSceneModel
    {
        std::shared_ptr<SModel> Model;
        glm::vec3               Position;
        glm::vec3               Scale;
        glm::vec3               Rotation;
    };

    SScene(CDevice* pDevice);
    virtual ~SScene();

    // IScene Interface
    virtual void Initialize() override;
    virtual void Reset() override;

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

    // Cache the device
    CDevice* m_pDevice;

    // Camera
    CCamera                    m_Camera;
    SSceneSettings             m_Settings;
    std::vector<SSceneModel>   m_ModelInstances;

    // Materials
    CSampler*                  m_pMaterialSampler;
    std::vector<SMaterialGLSL> m_GpuMaterials;

    // Vulkan Resources
    CAccelerationStructure*    m_pTopLevelAS;
    std::vector<SMeshInfo>     m_MeshInfoBuffer;
};

struct SceneFactory
{
    static SScene* CreateScene(ESceneType SceneType);
};