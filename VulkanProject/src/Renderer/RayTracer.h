#pragma once
#include "Core.h"
#include "IRenderer.h"
#include "Camera.h"

struct RandomBuffer
{
    uint32_t FrameIndex;
    uint32_t Padding0;
    uint32_t Padding1;
    uint32_t Padding2;
};

class RayTracer : public IRenderer
{
public:
    RayTracer();
    ~RayTracer() = default;
    
    virtual void Init(Device* pDevice, Swapchain* pSwapchain) override;
    
    virtual void Release() override;
    
    virtual void Tick(float deltaTime) override;
    
    virtual void OnRenderUI() override;
    
    virtual void OnWindowResize(uint32_t width, uint32_t height) override;
    
private:
    void CreateOrResizeSceneTexture(uint32_t width, uint32_t height);
    void CreateDescriptorSet();
    void ReleaseDescriptorSet();
    
    Device*                    m_pDevice;
    Swapchain*                 m_pSwapchain;
    class ComputePipeline*     m_Pipeline;
    class PipelineLayout*      m_pPipelineLayout;
    class DescriptorSetLayout* m_pDescriptorSetLayout;
    DeviceMemoryAllocator*     m_pDeviceAllocator;
    DescriptorPool*            m_pDescriptorPool;
    class DescriptorSet*       m_pDescriptorSet;

    std::vector<class CommandBuffer*> m_CommandBuffers;
    std::vector<class Query*>         m_TimestampQueries;
    
    class Buffer* m_pCameraBuffer;
    class Buffer* m_pRandomBuffer;
        
    class Texture*       m_pSceneTexture;
    class TextureView*   m_pSceneTextureView;
    class DescriptorSet* m_pSceneTextureDescriptorSet;

    Camera   m_Camera;
    float    m_LastGPUTime;
    uint32_t m_ViewportWidth;
    uint32_t m_ViewportHeight;
};
