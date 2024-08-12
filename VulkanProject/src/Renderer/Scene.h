#pragma once
#include "Camera.h"
#include "Model.h"
#include "IScene.h"
#include "Vulkan/AccelerationStructure.h"

class FSampler;

struct FMeshInfo
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

struct FSceneSettings
{
    EViewMode       ViewMode;
    EBackgroundType BackgroundType;
    float           GradientLightStrength;
    float           Exposure;
    uint32_t        NumBounces;
    float           FieldOfView;
    float           CameraSpeed;
};


struct FScene : public IScene
{
    struct FSceneModel
    {
        std::shared_ptr<FModel> Model;
        glm::vec3               Position;
        glm::vec3               Scale;
        glm::vec3               Rotation;
    };

    FScene(FDevice* pDevice);
    virtual ~FScene();

    // IScene Interface
    virtual void Initialize() override;
    virtual void Reset() override;

    virtual void OnRenderUI() override { }

    virtual float GetCameraSpeed() const override { return m_Settings.CameraSpeed; }
    virtual float GetFieldOfView() const override { return m_Settings.FieldOfView; }
    virtual float GetExposure() const override { return m_Settings.Exposure; }

    virtual FCamera& GetCamera() override { return m_Camera; }
    virtual const FCamera& GetCamera() const override { return m_Camera; }

    void AddModel(const std::shared_ptr<FModel>& Model, const glm::vec3& Position = glm::vec3(0.0f), const glm::vec3& Scale = glm::vec3(0.0f), const glm::vec3& Rotation = glm::vec3(0.0f))
    {
        m_ModelInstances.push_back({ Model, Position, Scale, Rotation });
    }

    // Cache the device
    FDevice* m_pDevice;

    // Camera
    FCamera                    m_Camera;
    FSceneSettings             m_Settings;
    std::vector<FSceneModel>   m_ModelInstances;

    // Materials
    FSampler*                  m_pMaterialSampler;
    std::vector<FMaterialGLSL> m_GpuMaterials;

    // Vulkan Resources
    FAccelerationStructure*    m_pTopLevelAS;
    std::vector<FMeshInfo>     m_MeshInfoBuffer;
};

struct FSceneFactory
{
    static FScene* CreateScene(ESceneType SceneType);
};