#pragma once
#include "Core.h"
#include "IRenderer.h"
#include "Camera.h"
#include "Scene.h"

class FBuffer;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Buffer Structs

struct FRandomBuffer
{
    uint32_t FrameIndex  = 0;
    uint32_t SampleIndex = 0;
    uint32_t NumSamples  = 0;
    uint32_t Padding0    = 0;
};

struct FSceneBuffer
{
    uint32_t NumQuads     = 0;
    uint32_t NumSpheres   = 0;
    uint32_t NumPlanes    = 0;
    uint32_t NumMaterials = 0;

    uint32_t BackgroundType = 0;
    float    Exposure       = 0.0f;
    uint32_t Padding0       = 0;
    uint32_t Padding1       = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// RayTracer

class FRayTracer : public IRenderer
{
public:
    FRayTracer();
    ~FRayTracer();
    
    virtual void Init(FDevice* pDevice, FSwapchain* pSwapchain) override;
    
    virtual void Release() override;
    
    virtual void Tick(float deltaTime) override;
    
    virtual void OnRenderUI() override;
    
    virtual void OnWindowResize(uint32_t width, uint32_t height) override;
    
private:
    void CreateOrResizeSceneTexture(uint32_t width, uint32_t height);

    void CreateDescriptorSet();
    void ReleaseDescriptorSet();

    void ReloadShader();

    FDevice*                       m_pDevice;
    FSwapchain*                    m_pSwapchain;
    std::atomic<FComputePipeline*> m_pPipeline;
    class FPipelineLayout*         m_pPipelineLayout;
    class FDescriptorSetLayout*    m_pDescriptorSetLayout;
    FDeviceMemoryAllocator*        m_pDeviceAllocator;
    FDescriptorPool*               m_pDescriptorPool;
    class FDescriptorSet*          m_pDescriptorSet;

    std::vector<class FCommandBuffer*> m_CommandBuffers;
    std::vector<class FQuery*>         m_TimestampQueries;
    
    // Buffers
    FBuffer* m_pCameraBuffer;
    FBuffer* m_pRandomBuffer;
    FBuffer* m_pSceneBuffer;
    FBuffer* m_pSphereBuffer;
    FBuffer* m_pPlaneBuffer;
    FBuffer* m_pQuadBuffer;
    FBuffer* m_pMaterialBuffer;

    // SceneTexture
    class FTexture*       m_pAccumulationTexture;
    class FTextureView*   m_pAccumulationTextureView;
    class FTexture*       m_pSceneTexture;
    class FTextureView*   m_pSceneTextureView;
    class FDescriptorSet* m_pSceneTextureDescriptorSet;

    // Skybox
    class FTextureResource* m_pSkybox;
    class FSampler*         m_pSkyboxSampler;
    
    // Scene
    FScene* m_pScene;

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
