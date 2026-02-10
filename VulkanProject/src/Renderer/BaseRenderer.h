#pragma once
#include "Core.h"
#include "IRenderer.h"
#include "Vulkan/DeviceMemoryAllocator.h"

class CQuery;
class CTexture;
class CDescriptorSet;
class CTextureView;
class CTextureResource;
class CSampler;
class CBuffer;
class CGraphicsPipeline;
class CComputePipeline;
class CRenderPass;
class CPipelineLayout;
class CDescriptorSetLayout;
class CFramebuffer;

struct STonemappingBuffer
{
    // 0-4
    float Exposure = 0.5f;

    // Padding
    uint32_t Padding0 = 0;
    uint32_t Padding1 = 0;
    uint32_t Padding2 = 0;
};

struct SRandomBuffer
{
    // 0-8
    uint32_t FrameIndex  = 0;
    uint32_t HaltonIndex = 0;

    // Padding
    uint32_t Padding0 = 0;
    uint32_t Padding1 = 0;
};

class CBaseRenderer : public IRenderer
{
public:
    CBaseRenderer();
    ~CBaseRenderer();

    // IRenderer Interface
    virtual void Init(CDevice* pDevice, CSwapchain* pSwapchain) override final;
    virtual void Release() override final;

    virtual void Tick(float DeltaTime) override final;

    virtual void OnRenderUI() override final;

    virtual CDevice* GetDevice() const override final { return m_pDevice; }
    virtual CDeviceMemoryAllocator* GetDeviceAllocator() const override final { return m_pDeviceAllocator; }
    virtual CDescriptorPool* GetDescriptorPool() const override final { return m_pDescriptorPool; }

    // BaseRenderer Interface
    virtual bool CreateOrResizeSceneTexture(uint32_t Width, uint32_t Height);

    virtual void CreateResources() = 0;
    virtual void ReleaseResources() = 0;

    virtual void RenderUI() = 0;
    virtual void ReloadShaders() = 0;

    virtual void CreateDescriptorSets();
    virtual void ReleaseDescriptorSets();

    virtual void Render(CCommandBuffer* pCommandBuffer) = 0;

    virtual void CreateGlobalBuffers();

    void CreateTonemappingResources();
    void PerformTonemapping(CCommandBuffer* pCommandBuffer);

    void ResetImage()
    {
        m_bResetImage = true;
    }

    uint64_t GetFrameIndex() const
    {
        return m_FrameIndex;
    }

    uint32_t GetViewportWidth() const
    {
        return m_ViewportWidth;
    }

    uint32_t GetViewportHeight() const
    {
        return m_ViewportHeight;
    }

protected:

    // Scene textures
    CTexture*         m_pSceneTexture0;
    CTextureView*     m_pSceneTextureView0;
    CTexture*         m_pSceneTexture1;
    CTextureView*     m_pSceneTextureView1;
    CTexture*         m_pOutputTexture;
    CTextureView*     m_pOutputTextureView;

    // Samplers
    CSampler*         m_pSkyboxSampler;

    // Buffers
    CBuffer*          m_pCameraBuffer;
    CBuffer*          m_pRandomBuffer;

    // Skybox
    CTextureResource* m_pSkybox;
    uint32_t          m_SkyboxBindlessIndex;

private:

    // Base Objects
    CDevice*                m_pDevice;
    CSwapchain*             m_pSwapchain;
    CDeviceMemoryAllocator* m_pDeviceAllocator;
    CDescriptorPool*        m_pDescriptorPool;

    // Frame-Data
    std::vector<CCommandBuffer*> m_CommandBuffers;
    std::vector<CQuery*>         m_TimestampQueries;

    // Samples
    std::atomic_bool m_bResetImage;
    uint64_t         m_FrameIndex;

    // Stats
    float m_LastCPUTime;
    float m_LastGPUTime;

    // Viewport
    uint32_t m_ViewportWidth;
    uint32_t m_ViewportHeight;
    bool     m_bViewportHasFocus;

    // Scene textures
    CDescriptorSet* m_pOutputTextureDescriptorSet;

    // Samplers
    CSampler* m_pTonemapSampler;

    // ToneMapping
    CBuffer*              m_pTonemappingBuffer;
    CGraphicsPipeline*    m_pTonemappingPipeline;
    CRenderPass*          m_pTonemappingRenderPass;
    CPipelineLayout*      m_pTonemappingPipelineLayout;
    CDescriptorSetLayout* m_pTonemappingDescriptorSetLayout;
    CDescriptorSet*       m_pTonemappingDescriptorSet0;
    CDescriptorSet*       m_pTonemappingDescriptorSet1;
    CFramebuffer*         m_pTonemappingFramebuffer;
};
