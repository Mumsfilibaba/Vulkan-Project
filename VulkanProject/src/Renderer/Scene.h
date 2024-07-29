#pragma once
#include "Camera.h"
#include "Model.h"
#include "IScene.h"
#include "Vulkan/AccelerationStructure.h"

struct FScene : public IScene
{
    FScene();
    virtual ~FScene();

    // IScene Interface
    virtual void Initialize() override;
    virtual void Reset() override { }

    virtual void OnRenderUI() override { }

    virtual float GetCameraSpeed() const override { return 1.0f; }
    virtual float GetFieldOfView() const override { return 90.0f; }
    virtual float GetExposure() const override { return 1.0f; }

    virtual FCamera& GetCamera() override { return m_Camera; }
    virtual const FCamera& GetCamera() const override { return m_Camera; }

    FCamera m_Camera;

    FAccelerationStructure* pBLAS;
};