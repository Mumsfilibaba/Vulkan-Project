#pragma once
#include "Core.h"
#include "IRenderer.h"
#include "Camera.h"
#include "Scene.h"
#include "Bvh.h"

class FBuffer;
class FDescriptorSet;
class FTexture;
class FTextureView;
class FPipelineLayout;
class FDescriptorSetLayout;
class FGraphicsPipeline;
class FRenderPass;
class FSampler;
class FCommandBuffer;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Buffer Structs

struct FRandomBuffer
{
    // 0-8
    uint32_t FrameIndex  = 0;
    uint32_t HaltonIndex = 0;

    // Padding
    uint32_t Padding0 = 0;
    uint32_t Padding1 = 0;
};

struct FSceneBuffer
{
    // 0-16
    uint32_t NumQuads = 0;
    uint32_t NumSpheres = 0;
    uint32_t NumMeshes = 0;
    uint32_t NumMaterials = 0;
    // 16-32
    uint32_t NumBvhNodes = 0;
    uint32_t NumTriangles = 0;
    uint32_t BackgroundType = 0;
    uint32_t NumBounces = 4;
    // 32-36
    uint32_t ViewMode = 0;
    
    // Padding
    uint32_t Padding0;
    uint32_t Padding1;
    uint32_t Padding2;
};

struct FTonemappingBuffer
{
    // 0-4
    float Exposure = 0.5f;
    
    // Padding
    uint32_t Padding0 = 0;
    uint32_t Padding1 = 0;
    uint32_t Padding2 = 0;
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
    void CreateRayTracingResources();
    void CreateTonemappingResources();
    void CreateGlobalBuffers();
    void CreateDescriptorSet();
    void ReleaseDescriptorSets();
    void CreateOrResizeSceneTexture(uint32_t width, uint32_t height);
    void ReloadShader();
    void UpdateGlobalBuffers(FCommandBuffer* pCommandBuffer);

    FDevice*                       m_pDevice;
    FSwapchain*                    m_pSwapchain;
    FDeviceMemoryAllocator*        m_pDeviceAllocator;
    FDescriptorPool*               m_pDescriptorPool;

    std::vector<FCommandBuffer*> m_CommandBuffers;
    std::vector<class FQuery*>   m_TimestampQueries;

    // RayTracing
    std::atomic<FComputePipeline*> m_pRayTracingPipeline;
    FPipelineLayout*               m_pRayTracingPipelineLayout;
    FDescriptorSetLayout*          m_pRayTracingDescriptorSetLayout;
    FDescriptorSet*                m_pRayTracingDescriptorSet0;
    FDescriptorSet*                m_pRayTracingDescriptorSet1;
    
    // ToneMapping
    FGraphicsPipeline*    m_pTonemappingPipeline;
    FRenderPass*          m_pTonemappingRenderPass;
    FPipelineLayout*      m_pTonemappingPipelineLayout;
    FDescriptorSetLayout* m_pTonemappingDescriptorSetLayout;
    FDescriptorSet*       m_pTonemappingDescriptorSet0;
    FDescriptorSet*       m_pTonemappingDescriptorSet1;
    class FFramebuffer*   m_pTonemappingFramebuffer;
    
    // Buffers
    FBuffer* m_pCameraBuffer;
    FBuffer* m_pRandomBuffer;
    FBuffer* m_pSceneBuffer;
    FBuffer* m_pTonemappingBuffer;
    FBuffer* m_pSphereBuffer;
    FBuffer* m_pQuadBuffer;
    FBuffer* m_pTriangleBuffer;
    FBuffer* m_pMeshBuffer;
    FBuffer* m_pVertexBuffer;
    FBuffer* m_pMaterialBuffer;
    FBuffer* m_pBvhBuffer;

    // SceneTexture
    FTexture*       m_pSceneTexture0;
    FTexture*       m_pSceneTexture1;
    FTexture*       m_pOutputTexture;
    FTextureView*   m_pSceneTextureView0;
    FTextureView*   m_pSceneTextureView1;
    FTextureView*   m_pOutputTextureView;
    FDescriptorSet* m_pOutputTextureDescriptorSet;

    // Skybox
    class FTextureResource* m_pSkybox;
    
    // Samplers
    FSampler* m_pSkyboxSampler;
    FSampler* m_pTonemapSampler;
    
    // Scene
    FScene* m_pScene;
    
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
