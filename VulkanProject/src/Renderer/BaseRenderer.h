#pragma once
#include "Core.h"
#include "IRenderer.h"
#include "Vulkan/DeviceMemoryAllocator.h"

class FQuery;
class FTexture;
class FDescriptorSet;
class FTextureView;
class FTextureResource;
class FSampler;
class FBuffer;
class FGraphicsPipeline;
class FComputePipeline;
class FRenderPass;
class FPipelineLayout;
class FDescriptorSetLayout;
class FFramebuffer;

struct FTonemappingBuffer
{
    // 0-4
    float Exposure = 0.5f;

    // Padding
    uint32_t Padding0 = 0;
    uint32_t Padding1 = 0;
    uint32_t Padding2 = 0;
};

struct FRandomBuffer
{
    // 0-8
    uint32_t FrameIndex  = 0;
    uint32_t HaltonIndex = 0;

    // Padding
    uint32_t Padding0 = 0;
    uint32_t Padding1 = 0;
};

class FBaseRenderer : public IRenderer
{
public:
    FBaseRenderer();
    ~FBaseRenderer();

    // IRenderer Interface
    virtual void Init(FDevice* pDevice, FSwapchain* pSwapchain) override;
    virtual void Release() override;

    virtual void Tick(float DeltaTime) override final;

    virtual void OnRenderUI() override final;

    virtual FDevice* GetDevice() const override final { return m_pDevice; }
    virtual FDeviceMemoryAllocator* GetDeviceAllocator() const override final { return m_pDeviceAllocator; }
    virtual FDescriptorPool* GetDescriptorPool() const override final { return m_pDescriptorPool; }

    // BaseRenderer Interface
    virtual bool CreateOrResizeSceneTexture(uint32_t Width, uint32_t Height);

    virtual void RenderSceneUI() = 0;
    virtual void ReloadShaders() = 0;

    virtual void CreateDescriptorSets();
    virtual void ReleaseDescriptorSets();

    virtual void Render(FCommandBuffer* pCommandBuffer) = 0;

    virtual void CreateGlobalBuffers();

    void CreateTonemappingResources();
    void PerformTonemapping(FCommandBuffer* pCommandBuffer);

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
    FTexture*         m_pSceneTexture0;
    FTextureView*     m_pSceneTextureView0;
    FTexture*         m_pSceneTexture1;
    FTextureView*     m_pSceneTextureView1;
    FTexture*         m_pOutputTexture;
    FTextureView*     m_pOutputTextureView;

    // Samplers
    FSampler*         m_pSkyboxSampler;

    // Buffers
    FBuffer*          m_pCameraBuffer;
    FBuffer*          m_pRandomBuffer;

    // Skybox
    FTextureResource* m_pSkybox;
    uint32_t          m_SkyboxBindlessIndex;

private:

    // Base Objects
    FDevice*                     m_pDevice;
    FSwapchain*                  m_pSwapchain;
    FDeviceMemoryAllocator*      m_pDeviceAllocator;
    FDescriptorPool*             m_pDescriptorPool;

    // Frame-Data
    std::vector<FCommandBuffer*> m_CommandBuffers;
    std::vector<FQuery*>         m_TimestampQueries;

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

    // Scene textures
    FDescriptorSet*       m_pOutputTextureDescriptorSet;

    // Samplers
    FSampler*             m_pTonemapSampler;

    // ToneMapping
    FGraphicsPipeline*    m_pTonemappingPipeline;
    FRenderPass*          m_pTonemappingRenderPass;
    FPipelineLayout*      m_pTonemappingPipelineLayout;
    FDescriptorSetLayout* m_pTonemappingDescriptorSetLayout;
    FDescriptorSet*       m_pTonemappingDescriptorSet0;
    FDescriptorSet*       m_pTonemappingDescriptorSet1;
    FFramebuffer*         m_pTonemappingFramebuffer;

    // Buffers
    FBuffer*              m_pTonemappingBuffer;
};
