#pragma once
#include "BaseRenderer.h"
#include "Scene.h"

class CRayTracingPipeline;

struct SSceneBuffer
{
    // 0-16
    uint32_t NumMaterials   = 0;
    uint32_t BackgroundType = 0;
    uint32_t NumBounces     = 4;
    uint32_t ViewMode       = 0;
    // 16-20
    float    GradientLightStrength = 1.0f;

    // Padding
    uint32_t Padding0;
    uint32_t Padding1;
    uint32_t Padding2;
};

class CRayTracer : public CBaseRenderer
{
public:
    CRayTracer();
    ~CRayTracer();

    // IRenderer Interface
    virtual IScene* GetScene() const override final
    {
        return m_pScene;
    }

    // BaseRenderer Interface
    virtual void CreateResources() override;
    virtual void ReleaseResources() override;

    virtual void CreateDescriptorSets() override;
    virtual void ReleaseDescriptorSets() override;

    virtual void RenderUI() override;
    virtual void ReloadShaders() override;

    virtual void Render(CCommandBuffer* pCommandBuffer) override;

    virtual void CreateGlobalBuffers() override;

private:
    void UpdateGlobalBuffers(CCommandBuffer* pCommandBuffer);

    // Current Scene
    SScene* m_pScene;
    
    // Buffers
    CBuffer* m_pSceneSettingsBuffer;
    CBuffer* m_pMaterialBuffer;
    CBuffer* m_pMeshBuffer;

    // RayTracing Pass
    CRayTracingPipeline*  m_pRayTracingPipeline;
    CPipelineLayout*      m_pRayTracingPipelineLayout;
    CDescriptorSetLayout* m_pRayTracingDescriptorSetLayout;
    CDescriptorSet*       m_pRayTracingDescriptorSet0;
    CDescriptorSet*       m_pRayTracingDescriptorSet1;
};
