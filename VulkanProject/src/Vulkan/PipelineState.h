#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>

class RenderPass;
class ShaderModule;
class PipelineLayout;

struct GraphicsPipelineStateParams
{
    VkVertexInputAttributeDescription* pAttributeDescriptions    = nullptr;
    uint32_t                           attributeDescriptionCount = 0;
    VkVertexInputBindingDescription*   pBindingDescriptions      = nullptr;
    uint32_t                           bindingDescriptionCount   = 0;

    VkCullModeFlagBits cullMode  = VK_CULL_MODE_BACK_BIT;
    VkFrontFace        frontFace = VK_FRONT_FACE_CLOCKWISE;
    
    bool bBlendEnable = false;
    
    RenderPass*     pRenderPass     = nullptr;
    PipelineLayout* pPipelineLayout = nullptr;
    ShaderModule*   pVertexShader   = nullptr;
    ShaderModule*   pFragmentShader = nullptr;
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
    
protected:
    VkDevice   m_Device;
    VkPipeline m_Pipeline;
};

class GraphicsPipeline : public BasePipeline
{
public:
    static GraphicsPipeline* Create(class Device* pDevice, const GraphicsPipelineStateParams& params);
    
    GraphicsPipeline(VkDevice device);
    ~GraphicsPipeline() = default;
};

struct ComputePipelineStateParams
{
    ShaderModule*   pShader         = nullptr;
    PipelineLayout* pPipelineLayout = nullptr;
};

class ComputePipeline : public BasePipeline
{
public:
    static ComputePipeline* Create(class Device* pDevice, const ComputePipelineStateParams& params);
    
    ComputePipeline(VkDevice device);
    ~ComputePipeline() = default;
};
