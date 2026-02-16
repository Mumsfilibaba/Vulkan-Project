#pragma once
#include "BaseRenderer.h"
#include "Camera.h"
#include "SoftwareScene.h"
#include "Bvh.h"

struct SSoftwareSceneBuffer
{
    // 0-16
    uint32_t NumQuads     = 0;
    uint32_t NumSpheres   = 0;
    uint32_t NumMeshes    = 0;
    uint32_t NumMaterials = 0;
    // 16-32
    uint32_t NumBvhNodes    = 0;
    uint32_t NumTlasNodes   = 0;
    uint32_t NumTriangles   = 0;
    uint32_t BackgroundType = 0;
    // 32-48
    uint32_t NumBounces            = 4;
    uint32_t ViewMode              = 0;
    float    GradientLightStrength = 1.0f;
    
    // Padding
    uint32_t Padding0 = 0;
    uint32_t Padding1 = 0;
    uint32_t Padding2 = 0;
};

class CSoftwareRayTracer : public CBaseRenderer
{
public:
    CSoftwareRayTracer();
    ~CSoftwareRayTracer();

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

    virtual void Render(CCommandBuffer* pCommandBuffer) override;

    virtual void CreateGlobalBuffers() override;

private:
    void CreateRayTracingResources();
    void CreateDebugViewResources();
    void UpdateGlobalBuffers(CCommandBuffer* pCommandBuffer);
    void PerformRayTracing(CCommandBuffer* pCommandBuffer);
    void PerformDebugPass(CCommandBuffer* pCommandBuffer);

    // Scene
    SSoftwareScene* m_pScene;

    // Buffers
    CBuffer* m_pSceneSettingsBuffer;
    CBuffer* m_pMaterialBuffer;
    CBuffer* m_pSphereBuffer;
    CBuffer* m_pQuadBuffer;
    CBuffer* m_pTriangleBuffer;
    CBuffer* m_pMeshBuffer;
    CBuffer* m_pVertexPositionsBuffer;
    CBuffer* m_pVertexBuffer;
    CBuffer* m_pIndexBuffer;
    CBuffer* m_pBvhBuffer;
    CBuffer* m_pTlasBuffer;

    CBuffer* m_pAABBVertexBuffer;
    CBuffer* m_pAABBIndexBuffer;
    CBuffer* m_pAABBInstanceBuffer;
    size_t   m_AABBIndexCount;

    // RayTracing Pass
    std::atomic<CComputePipeline*> m_pRayTracingPipeline;
    CPipelineLayout*               m_pRayTracingPipelineLayout;
    CDescriptorSetLayout*          m_pRayTracingDescriptorSetLayout;
    CDescriptorSet*                m_pRayTracingDescriptorSet0;
    CDescriptorSet*                m_pRayTracingDescriptorSet1;

    // DebugPass
    CGraphicsPipeline*    m_pDebugPipeline;
    CGraphicsPipeline*    m_pDebugPipelineWireframe;
    CGraphicsPipeline*    m_pDebugAABBPipeline;
    CPipelineLayout*      m_pDebugPipelineLayout;
    CPipelineLayout*      m_pDebugAABBPipelineLayout;
    CDescriptorSetLayout* m_pDebugDescriptorSetLayout;
    CDescriptorSet*       m_pDebugDescriptorSet0;
    CDescriptorSet*       m_pDebugDescriptorSet1;
    CTexture*             m_pDepthBufferTexture;
    CTextureView*         m_pDepthBufferTextureView;
};
