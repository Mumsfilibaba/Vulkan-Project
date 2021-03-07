#pragma once
#include "Core.h"

#include <vulkan/vulkan.h>

class RenderPass;
class ShaderModule;

struct GraphicsPipelineStateParams
{
	VkVertexInputAttributeDescription* pAttributeDescriptions = nullptr;
	uint32_t AttributeDescriptionCount = 0;
	VkVertexInputBindingDescription* pBindingDescriptions = nullptr;
	uint32_t BindingDescriptionCount = 0;
	RenderPass* pRenderPass	= nullptr;
	ShaderModule* pVertex   = nullptr;
	ShaderModule* pFragment	= nullptr;
};

class BasePipeline
{
public:
	BasePipeline(VkDevice device);
	~BasePipeline();
	
	inline VkPipeline GetPipeline() const
	{
		return m_Pipeline;
	}
	
	inline VkPipelineLayout GetPiplineLayout() const
	{
		return m_Layout;
	}
	
	inline VkDescriptorSetLayout GetDescriptorSetLayout() const
	{
		return m_DescriptorSetLayout;
	}
	
protected:
	VkDevice 	     m_Device;
	VkPipeline 		 m_Pipeline;
	VkPipelineLayout m_Layout;
	VkDescriptorSetLayout m_DescriptorSetLayout;
};

class GraphicsPipeline : public BasePipeline
{
public:
	GraphicsPipeline(VkDevice device);
	~GraphicsPipeline() = default;

	static GraphicsPipeline* Create(class VulkanContext* pContext, const GraphicsPipelineStateParams& params);
};

struct ComputePipelineStateParams
{
	ShaderModule* pShader = nullptr;
};

class ComputePipeline : public BasePipeline
{
public:
	ComputePipeline(VkDevice device);
	~ComputePipeline() = default;
	
	static ComputePipeline* Create(class VulkanContext* pContext, const ComputePipelineStateParams& params);
};
