#pragma once
#include "Model.h"
#include "Camera.h"
#include "IRenderer.h"

class Renderer : public IRenderer
{
public:
    Renderer();
    ~Renderer() = default;
    
    virtual void Init(VulkanContext* pContext) override;
    
    virtual void Release() override;
    
    virtual void Tick(float deltaTime) override;
    
    virtual void OnRenderUI() {}
    
    virtual void OnWindowResize(uint32_t width, uint32_t height) override;
    
private:
    void CreateFramebuffers();
    void ReleaseFramebuffers();

    VulkanContext*          m_pContext;
    class RenderPass*       m_pRenderPass;
    class GraphicsPipeline* m_PipelineState;
    class CommandBuffer*    m_pCurrentCommandBuffer;
    VulkanDeviceAllocator*  m_pDeviceAllocator;
    DescriptorPool*         m_pDescriptorPool;
    
    std::vector<class Framebuffer*>   m_Framebuffers;
    std::vector<class CommandBuffer*> m_CommandBuffers;
    
    class Buffer* m_pCameraBuffer;
    
    Model* m_pModel;
    Camera m_Camera;
};
