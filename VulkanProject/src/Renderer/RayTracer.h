#pragma once
#include "BaseRenderer.h"
#include "Scene.h"

class FRayTracingPipeline;

struct FSceneBuffer
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

class FRayTracer : public FBaseRenderer
{
public:
    FRayTracer();
    ~FRayTracer();

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

    virtual void Render(FCommandBuffer* pCommandBuffer) override;

    virtual void CreateGlobalBuffers() override;

private:
    void UpdateGlobalBuffers(FCommandBuffer* pCommandBuffer);

    // Current Scene
    FScene* m_pScene;
    
    // Buffers
    FBuffer* m_pSceneSettingsBuffer;
    FBuffer* m_pMaterialBuffer;
    FBuffer* m_pMeshBuffer;

    // RayTracing Pass
    FRayTracingPipeline*  m_pRayTracingPipeline;
    FPipelineLayout*      m_pRayTracingPipelineLayout;
    FDescriptorSetLayout* m_pRayTracingDescriptorSetLayout;
    FDescriptorSet*       m_pRayTracingDescriptorSet0;
    FDescriptorSet*       m_pRayTracingDescriptorSet1;
};
