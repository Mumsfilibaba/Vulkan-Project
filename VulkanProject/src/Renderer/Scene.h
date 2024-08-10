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

    // Cache the device
    FDevice* m_pDevice;

    // Camera
    FCamera        m_Camera;
    FSceneSettings m_Settings;

    // Vulkan Resources
    FAccelerationStructure*              m_pTopLevelAS;
    std::vector<FBuffer*>                m_VertexBuffers;
    std::vector<FBuffer*>                m_IndexBuffers;
    std::vector<FAccelerationStructure*> m_BottomLevelASs;
    std::vector<FMeshInfo>               m_MeshInfoBuffer;

    // Materials
    FSampler*                    m_pMaterialSampler;
    std::vector<FMaterial>       m_Materials;
    std::vector<FMaterialGLSL> m_GpuMaterials;
};