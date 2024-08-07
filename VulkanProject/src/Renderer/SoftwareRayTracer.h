#pragma once
#include "BaseRenderer.h"
#include "Camera.h"
#include "SoftwareScene.h"
#include "Bvh.h"

struct FSoftwareSceneBuffer
{
    // 0-16
    uint32_t NumQuads     = 0;
    uint32_t NumSpheres   = 0;
    uint32_t NumMeshes    = 0;
    uint32_t NumMaterials = 0;
    // 16-32
    uint32_t NumBvhNodes    = 0;
    uint32_t NumTriangles   = 0;
    uint32_t BackgroundType = 0;
    uint32_t NumBounces     = 4;
    // 32-40
    uint32_t ViewMode              = 0;
    float    GradientLightStrength = 1.0f;
    
    // Padding
    uint32_t Padding0;
    uint32_t Padding1;
};

class FSoftwareRayTracer : public FBaseRenderer
{
public:
    FSoftwareRayTracer();
    ~FSoftwareRayTracer();

    // IRenderer Interface
    virtual IScene* GetScene() const override final
    {
        return m_pScene;
    }

    // BaseRenderer Interface
    virtual bool CreateOrResizeSceneTexture(uint32_t Width, uint32_t Height) override;

    virtual void CreateResources() override;
    virtual void ReleaseResources() override;

    virtual void CreateDescriptorSets() override;
    virtual void ReleaseDescriptorSets() override;

    virtual void RenderUI() override;
    virtual void ReloadShaders() override;

    virtual void Render(FCommandBuffer* pCommandBuffer) override;

    virtual void CreateGlobalBuffers() override;

private:
    void CreateRayTracingResources();
    void CreateDebugViewResources();
    void UpdateGlobalBuffers(FCommandBuffer* pCommandBuffer);
    void PerformRayTracing(FCommandBuffer* pCommandBuffer);
    void PerformDebugPass(FCommandBuffer* pCommandBuffer);

    // Scene
    FSoftwareScene* m_pScene;

    // Buffers
    FBuffer* m_pSceneSettingsBuffer;
    FBuffer* m_pMaterialBuffer;
    FBuffer* m_pSphereBuffer;
    FBuffer* m_pQuadBuffer;
    FBuffer* m_pTriangleBuffer;
    FBuffer* m_pMeshBuffer;
    FBuffer* m_pVertexBuffer;
    FBuffer* m_pVertexExBuffer;
    FBuffer* m_pBvhBuffer;
    FBuffer* m_pAABBVertexBuffer;
    FBuffer* m_pAABBIndexBuffer;
    FBuffer* m_pAABBInstanceBuffer;

    // RayTracing Pass
    std::atomic<FComputePipeline*> m_pRayTracingPipeline;
    FPipelineLayout*               m_pRayTracingPipelineLayout;
    FDescriptorSetLayout*          m_pRayTracingDescriptorSetLayout;
    FDescriptorSet*                m_pRayTracingDescriptorSet0;
    FDescriptorSet*                m_pRayTracingDescriptorSet1;

    // DebugPass
    size_t                m_AABBIndexCount;
    FGraphicsPipeline*    m_pDebugPipeline;
    FGraphicsPipeline*    m_pDebugPipelineWireframe;
    FGraphicsPipeline*    m_pDebugAABBPipeline;
    FRenderPass*          m_pDebugRenderPass;
    FPipelineLayout*      m_pDebugPipelineLayout;
    FPipelineLayout*      m_pDebugAABBPipelineLayout;
    FDescriptorSetLayout* m_pDebugDescriptorSetLayout;
    FDescriptorSet*       m_pDebugDescriptorSet0;
    FDescriptorSet*       m_pDebugDescriptorSet1;
    FFramebuffer*         m_pDebugFramebuffer;
    FTexture*             m_pDepthBufferTexture;
    FTextureView*         m_pDepthBufferTextureView;
};
