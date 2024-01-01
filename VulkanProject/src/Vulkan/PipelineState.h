#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>

class RenderPass;
class ShaderModule;

struct GraphicsPipelineStateParams
{
    VkVertexInputAttributeDescription* pAttributeDescriptions    = nullptr;
    uint32_t                           AttributeDescriptionCount = 0;
    VkVertexInputBindingDescription*   pBindingDescriptions      = nullptr;
    uint32_t                           BindingDescriptionCount   = 0;
    RenderPass*   pRenderPass = nullptr;
    ShaderModule* pVertex     = nullptr;
    ShaderModule* pFragment   = nullptr;
};

class BasePipeline
{
public:
    BasePipeline(VkDevice device);
    ~BasePipeline();
    
    VkPipeline GetPipeline() const
    {
        return m_Pipeline;
    }
    
    VkPipelineLayout GetPiplineLayout() const
    {
        return m_Layout;
    }
    
    VkDescriptorSetLayout GetDescriptorSetLayout() const
    {
        return m_DescriptorSetLayout;
    }
    
protected:
    VkDevice              m_Device;
    VkPipeline            m_Pipeline;
    VkPipelineLayout      m_Layout;
    VkDescriptorSetLayout m_DescriptorSetLayout;
};

class GraphicsPipeline : public BasePipeline
{
public:
    static GraphicsPipeline* Create(class VulkanContext* pContext, const GraphicsPipelineStateParams& params);
    
    GraphicsPipeline(VkDevice device);
    ~GraphicsPipeline() = default;
};

struct ComputePipelineStateParams
{
    ShaderModule* pShader = nullptr;
};

class ComputePipeline : public BasePipeline
{
public:
    static ComputePipeline* Create(class VulkanContext* pContext, const ComputePipelineStateParams& params);
    
    ComputePipeline(VkDevice device);
    ~ComputePipeline() = default;
};
