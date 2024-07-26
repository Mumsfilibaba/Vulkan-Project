#pragma once
#include "Core.h"
#include "IRenderer.h"
#include "Vulkan/DeviceMemoryAllocator.h"

class FQuery;

class FBaseRenderer : public IRenderer
{
public:
    FBaseRenderer();
    ~FBaseRenderer();

    // IRenderer Interface
    virtual void Init(FDevice* pDevice, FSwapchain* pSwapchain) override;
    virtual void Release() override;
    virtual void Tick(float DeltaTime) override;

    virtual void OnRenderUI() override;
    virtual void OnWindowResize(uint32_t Width, uint32_t Height) override;

    // BaseRenderer Interface
    virtual void RenderSceneUI() = 0;

protected:
    FDevice*                     m_pDevice;
    FSwapchain*                  m_pSwapchain;
    FDeviceMemoryAllocator*      m_pDeviceAllocator;
    FDescriptorPool*             m_pDescriptorPool;
    std::vector<FCommandBuffer*> m_CommandBuffers;
    std::vector<FQuery*>         m_TimestampQueries;

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
