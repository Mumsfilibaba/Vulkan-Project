#pragma once
#include "BaseRenderer.h"

class FRayTracer : public FBaseRenderer
{
public:
    FRayTracer();
    ~FRayTracer();

    // IRenderer Interface
    virtual void Init(FDevice* pDevice, FSwapchain* pSwapchain) override;
    virtual void Release() override;

    virtual IScene* GetScene() const override final
    {
        return nullptr;
    }

    // BaseRenderer Interface
    virtual void RenderSceneUI() override;
    virtual void ReloadShaders() override;
};
