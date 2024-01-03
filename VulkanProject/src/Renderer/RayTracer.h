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
    
    virtual void Init(VulkanContext* pContext) override;
    
    virtual void Release() override;
    
    virtual void Tick(float deltaTime) override;
    
    virtual void OnWindowResize(uint32_t width, uint32_t height) override;
    
private:
    void CreateDescriptorSets();
    void ReleaseDescriptorSets();
    
    VulkanContext*             m_pContext;
    class ComputePipeline*     m_Pipeline;
    class PipelineLayout*      m_pPipelineLayout;
    class DescriptorSetLayout* m_pDescriptorSetLayout;
    VulkanDeviceAllocator*     m_pDeviceAllocator;
    DescriptorPool*            m_pDescriptorPool;
    
    std::vector<class DescriptorSet*> m_DescriptorSets;
    std::vector<class CommandBuffer*> m_CommandBuffers;
    std::vector<class Query*>         m_TimestampQueries;
    
    Camera        m_Camera;
    
    class Buffer* m_pCameraBuffer;
    class Buffer* m_pRandomBuffer;
};
