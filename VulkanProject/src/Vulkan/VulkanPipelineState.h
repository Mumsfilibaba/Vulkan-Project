#pragma once
#include "Core.h"

#include <vulkan/vulkan.h>

class VulkanRenderPass;
class VulkanShaderModule;

struct GraphicsPipelineStateParams
{
	VulkanRenderPass* pRenderPass	= nullptr;
	VulkanShaderModule* pVertex		= nullptr;
	VulkanShaderModule* pFragment	= nullptr;
};

class VulkanGraphicsPipelineState
{
public:
	DECL_NO_COPY(VulkanGraphicsPipelineState);

	VulkanGraphicsPipelineState(VkDevice device, const GraphicsPipelineStateParams& params);
	~VulkanGraphicsPipelineState();

	VkPipeline GetPipeline() const { return m_Pipeline; }
	VkPipelineLayout GetPiplineLayout() const { return m_Layout; }
private:
	void Init(const GraphicsPipelineStateParams& params);
private:
	VkDevice m_Device;
	VkPipeline m_Pipeline;
	VkPipelineLayout m_Layout;
};