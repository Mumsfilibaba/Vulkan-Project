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
};

struct FScene : public IScene
{
    FScene(FDevice* pDevice);
    virtual ~FScene();

    // IScene Interface
    virtual void Initialize() override;
    virtual void Reset() override { }

    virtual void OnRenderUI() override { }

    virtual float GetCameraSpeed() const override { return m_CameraSpeed; }
    virtual float GetFieldOfView() const override { return 90.0f; }
    virtual float GetExposure() const override { return 1.0f; }

    virtual FCamera& GetCamera() override { return m_Camera; }
    virtual const FCamera& GetCamera() const override { return m_Camera; }

    // Cache the device
    FDevice* m_pDevice;

    FCamera m_Camera;
    float   m_CameraSpeed;

    // Vulkan Resources
    FAccelerationStructure*              m_pTopLevelAS;
    std::vector<FBuffer*>                m_VertexBuffers;
    std::vector<FBuffer*>                m_IndexBuffers;
    std::vector<FAccelerationStructure*> m_BottomLevelASs;
    std::vector<FMeshInfo>               m_MeshInfoBuffer;

    // Materials
    FBuffer*  m_pMaterialBuffer;
    FSampler* m_pMaterialSampler;
};