#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>

class FRenderPass;
class FShaderModule;
class FPipelineLayout;

struct FGraphicsPipelineStateParams
{
    VkVertexInputAttributeDescription* pAttributeDescriptions    = nullptr;
    uint32_t                           attributeDescriptionCount = 0;
    VkVertexInputBindingDescription*   pBindingDescriptions      = nullptr;
    uint32_t                           bindingDescriptionCount   = 0;

    VkCullModeFlagBits cullMode  = VK_CULL_MODE_BACK_BIT;
    VkFrontFace        frontFace = VK_FRONT_FACE_CLOCKWISE;
    
    bool bBlendEnable = false;
    
    FRenderPass*     pRenderPass     = nullptr;
    FPipelineLayout* pPipelineLayout = nullptr;
    FShaderModule*   pVertexShader   = nullptr;
    FShaderModule*   pFragmentShader = nullptr;
};

class FBasePipeline
{
public:
    FBasePipeline(VkDevice device);
    ~FBasePipeline();
    
    VkPipeline GetPipeline() const
    {
        return m_Pipeline;
    }
    
protected:
    VkDevice   m_Device;
    VkPipeline m_Pipeline;
};

class FGraphicsPipeline : public FBasePipeline
{
public:
    static FGraphicsPipeline* Create(class FDevice* pDevice, const FGraphicsPipelineStateParams& params);
    
    FGraphicsPipeline(VkDevice device);
    ~FGraphicsPipeline() = default;
};

struct FComputePipelineStateParams
{
    FShaderModule*   pShader         = nullptr;
    FPipelineLayout* pPipelineLayout = nullptr;
};

class FComputePipeline : public FBasePipeline
{
public:
    static FComputePipeline* Create(class FDevice* pDevice, const FComputePipelineStateParams& params);
    
    FComputePipeline(VkDevice device);
    ~FComputePipeline() = default;
};
