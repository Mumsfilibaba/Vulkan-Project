#pragma once
#include "Model.h"
#include "Camera.h"

#include "Vulkan/VulkanContext.h"
#include "Vulkan/VulkanDescriptorPool.h"

class Renderer
{
public:
	Renderer();
	~Renderer() = default;
	
	void Init(VulkanContext* pContext);
	void Release();
	
	// dt is in seconds
	void Tick(float dt);
	
	void OnWindowResize(uint32_t width, uint32_t height);
	
private:
	void CreateFramebuffers();
	void ReleaseFramebuffers();
	
private:
	VulkanContext* m_pContext;
	VulkanRenderPass* m_pRenderPass;
	VulkanGraphicsPipelineState* m_PipelineState;
	VulkanCommandBuffer* m_pCurrentCommandBuffer;
	VulkanDeviceAllocator* m_pDeviceAllocator;
	VulkanDescriptorPool* m_pDescriptorPool;
	
	std::vector<VulkanFramebuffer*> m_Framebuffers;
	std::vector<VulkanCommandBuffer*> m_CommandBuffers;
	
	VulkanBuffer* m_pCameraBuffer;
	
	Model* m_pModel;
	Camera m_Camera;
};
