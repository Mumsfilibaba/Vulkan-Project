#pragma once
#include "BaseRenderer.h"
#include "Scene.h"

class FRayTracingPipeline;

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
    virtual void Render(FCommandBuffer* pCommandBuffer) override;

    virtual void CreateResources() override;
    virtual void ReleaseResources() override;

    virtual void CreateDescriptorSets() override;
    virtual void ReleaseDescriptorSets() override;

    virtual void RenderSceneUI() override;
    virtual void ReloadShaders() override;

    virtual void CreateGlobalBuffers() override;

private:
    void UpdateGlobalBuffers(FCommandBuffer* pCommandBuffer);
    
    FScene*               m_pScene;

    // RayTracing
    FBuffer*              m_pMeshBuffer;
    FRayTracingPipeline*  m_pRayTracingPipeline;
    FPipelineLayout*      m_pRayTracingPipelineLayout;
    FDescriptorSetLayout* m_pRayTracingDescriptorSetLayout;
    FDescriptorSet*       m_pRayTracingDescriptorSet0;
    FDescriptorSet*       m_pRayTracingDescriptorSet1;
};
