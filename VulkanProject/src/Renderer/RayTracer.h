#pragma once
#include "Core.h"
#include "IRenderer.h"

#include "Camera.h"

class RayTracer : public IRenderer
{
public:
	RayTracer();
	~RayTracer() = default;
	
	virtual void Init(VulkanContext* pContext) override;
	virtual void Release() override;
	
	// dt is in seconds
	virtual void Tick(float dt) override;
	
	virtual void OnWindowResize(uint32_t width, uint32_t height) override;
	
private:
	void CreateDescriptorSets();
	void ReleaseDescriptorSets();
	
private:
	VulkanContext* m_pContext;
	class ComputePipeline* m_Pipeline;
	class CommandBuffer* m_pCurrentCommandBuffer;
	VulkanDeviceAllocator* m_pDeviceAllocator;
	DescriptorPool* m_pDescriptorPool;
	
	std::vector<class DescriptorSet*> m_DescriptorSets;
	std::vector<class CommandBuffer*> m_CommandBuffers;
	
	class Buffer* m_pCameraBuffer;
	
	Camera m_Camera;
};
