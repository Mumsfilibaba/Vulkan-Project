#pragma once
#include "Model.h"
#include "Camera.h"
#include "IRenderer.h"

class FRenderer : public IRenderer
{
public:
    FRenderer();
    ~FRenderer() = default;
    
    virtual void Init(FDevice* pDevice, FSwapchain* pSwapchain) override;
    
    virtual void Release() override;
    
    virtual void Tick(float deltaTime) override;
    
    virtual void OnRenderUI() {}
    
    virtual void OnWindowResize(uint32_t width, uint32_t height) override;
    
private:
    void CreateFramebuffers();
    void ReleaseFramebuffers();

    FDevice*                 m_pDevice;
    FSwapchain*              m_pSwapchain;
    class FRenderPass*       m_pRenderPass;
    class FGraphicsPipeline* m_PipelineState;
    class FCommandBuffer*    m_pCurrentCommandBuffer;
    FDeviceMemoryAllocator*  m_pDeviceAllocator;
    FDescriptorPool*         m_pDescriptorPool;
    
    std::vector<class FFramebuffer*>   m_Framebuffers;
    std::vector<class FCommandBuffer*> m_CommandBuffers;
    
    class FBuffer* m_pCameraBuffer;
    
    FModel* m_pModel;
    FCamera m_Camera;
};
