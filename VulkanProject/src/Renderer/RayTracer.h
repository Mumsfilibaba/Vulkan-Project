#pragma once
#include "Core.h"
#include "IRenderer.h"
#include "Camera.h"
#include "Scene.h"

class Buffer;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Buffer Structs

struct RandomBuffer
{
    uint32_t FrameIndex  = 0;
    uint32_t SampleIndex = 0;
    uint32_t NumSamples  = 0;
    uint32_t Padding0    = 0;
};

struct SceneBuffer
{
    uint32_t NumQuads     = 0;
    uint32_t NumSpheres   = 0;
    uint32_t NumPlanes    = 0;
    uint32_t NumMaterials = 0;

    uint32_t bUseGlobalLight = 0;
    uint32_t Padding0        = 0;
    uint32_t Padding1        = 0;
    uint32_t Padding2        = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// RayTracer

class RayTracer : public IRenderer
{
public:
    RayTracer();
    ~RayTracer();
    
    virtual void Init(Device* pDevice, Swapchain* pSwapchain) override;
    
    virtual void Release() override;
    
    virtual void Tick(float deltaTime) override;
    
    virtual void OnRenderUI() override;
    
    virtual void OnWindowResize(uint32_t width, uint32_t height) override;
    
private:
    void CreateOrResizeSceneTexture(uint32_t width, uint32_t height);

    void CreateDescriptorSet();
    void ReleaseDescriptorSet();

    void ReloadShader();

    Device*                       m_pDevice;
    Swapchain*                    m_pSwapchain;
    std::atomic<ComputePipeline*> m_pPipeline;
    class PipelineLayout*         m_pPipelineLayout;
    class DescriptorSetLayout*    m_pDescriptorSetLayout;
    DeviceMemoryAllocator*        m_pDeviceAllocator;
    DescriptorPool*               m_pDescriptorPool;
    class DescriptorSet*          m_pDescriptorSet;

    std::vector<class CommandBuffer*> m_CommandBuffers;
    std::vector<class Query*>         m_TimestampQueries;
    
    // Buffers
    Buffer* m_pCameraBuffer;
    Buffer* m_pRandomBuffer;
    Buffer* m_pSceneBuffer;
    Buffer* m_pSphereBuffer;
    Buffer* m_pPlaneBuffer;
    Buffer* m_pQuadBuffer;
    Buffer* m_pMaterialBuffer;

    // SceneTexture
    class Texture*       m_pAccumulationTexture;
    class TextureView*   m_pAccumulationTextureView;
    class Texture*       m_pSceneTexture;
    class TextureView*   m_pSceneTextureView;
    class DescriptorSet* m_pSceneTextureDescriptorSet;

    // Scene
    Scene* m_pScene;

    // Samples
    uint32_t         m_NumSamples;
    std::atomic_bool m_bResetImage;

    // Stats
    float m_LastCPUTime;
    float m_LastGPUTime;

    // Viewport
    uint32_t m_ViewportWidth;
    uint32_t m_ViewportHeight;
};
