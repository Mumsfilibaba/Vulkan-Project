#pragma once
#include "DeviceChild.h"
#include <vulkan/vulkan.h>

class FRenderPass;
class FShaderModule;
class FPipelineLayout;

struct FGraphicsPipelineStateParams
{
    VkVertexInputAttributeDescription* pAttributeDescriptions    = nullptr;
    uint32_t                           AttributeDescriptionCount = 0;
    VkVertexInputBindingDescription*   pBindingDescriptions      = nullptr;
    uint32_t                           BindingDescriptionCount   = 0;

    VkCullModeFlagBits CullMode        = VK_CULL_MODE_BACK_BIT;
    VkFrontFace        FrontFace       = VK_FRONT_FACE_CLOCKWISE;
    bool               bBlendEnable    = false;
    FRenderPass*       pRenderPass     = nullptr;
    FPipelineLayout*   pPipelineLayout = nullptr;
    FShaderModule*     pVertexShader   = nullptr;
    FShaderModule*     pFragmentShader = nullptr;
};

class FBasePipeline : public FDeviceChild
{
public:
    FBasePipeline(FDevice* pDevice);
    ~FBasePipeline();
    
    VkPipeline GetPipeline() const
    {
        return m_Pipeline;
    }
    
protected:
    VkPipeline m_Pipeline;
};

class FGraphicsPipeline : public FBasePipeline
{
public:
    static FGraphicsPipeline* Create(FDevice* pDevice, const FGraphicsPipelineStateParams& Params);
    
    FGraphicsPipeline(FDevice* pDevice);
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
    
    FComputePipeline(FDevice* pDevice);
    ~FComputePipeline() = default;
};
