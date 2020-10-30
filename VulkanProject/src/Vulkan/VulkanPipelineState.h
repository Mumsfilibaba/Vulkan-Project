#pragma once
#include "Core.h"

#include <vulkan/vulkan.h>

class VulkanRenderPass;
class VulkanShaderModule;

struct GraphicsPipelineStateParams
{
	VkVertexInputAttributeDescription* pAttributeDescriptions = nullptr;
	uint32_t AttributeDescriptionCount = 0;
	VkVertexInputBindingDescription* pBindingDescriptions = nullptr;
	uint32_t BindingDescriptionCount = 0;
	VulkanRenderPass* pRenderPass	= nullptr;
	VulkanShaderModule* pVertex		= nullptr;
	VulkanShaderModule* pFragment	= nullptr;
};

class VulkanGraphicsPipelineState
{
public:
	VulkanGraphicsPipelineState(VkDevice device, const GraphicsPipelineStateParams& params);
	~VulkanGraphicsPipelineState();

	inline VkPipeline GetPipeline() const
	{
		return m_Pipeline;
	}
	
	inline VkPipelineLayout GetPiplineLayout() const
	{
		return m_Layout;
	}
	
private:
	void Init(const GraphicsPipelineStateParams& params);
	
private:
	VkDevice m_Device;
	VkPipeline m_Pipeline;
	VkPipelineLayout m_Layout;
};
