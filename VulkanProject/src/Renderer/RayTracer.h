#pragma once
#include "BaseRenderer.h"

class FRayTracer : public FBaseRenderer
{
public:
    FRayTracer();
    ~FRayTracer();

    virtual void Init(FDevice* pDevice, FSwapchain* pSwapchain) override;
    virtual void Release() override;
    virtual void Tick(float DeltaTime) override;

    virtual void RenderSceneUI() override;

private:
    // Samples
    std::atomic_bool m_bResetImage;
    uint64_t         m_FrameIndex;
    
    // Stats
    float    m_LastCPUTime;
    float    m_LastGPUTime;
    
    // Viewport
    uint32_t m_ViewportWidth;
    uint32_t m_ViewportHeight;
    bool     m_bViewportHasFocus;
};
