#include "PipelineState.h"
#include "ShaderModule.h"
#include "RenderPass.h"
#include "Device.h"
#include "PipelineLayout.h"
#include "Extensions.h"

FBasePipeline::FBasePipeline(FDevice* pDevice)
    : FDeviceChild(pDevice)
    , m_Pipeline(VK_NULL_HANDLE)
{
}

FBasePipeline::~FBasePipeline()
{
    if (m_Pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(GetDevice()->GetDevice(), m_Pipeline, nullptr);
        m_Pipeline = VK_NULL_HANDLE;
    }
}

void FBasePipeline::SetDebugName(const char* DebugName)
{
    if (FExtensions::vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT DebugNameInfo;
        ZERO_STRUCT(&DebugNameInfo);
        
        DebugNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        DebugNameInfo.objectType   = VK_OBJECT_TYPE_PIPELINE;
        DebugNameInfo.pObjectName  = DebugName;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_Pipeline);

        VkResult Result = FExtensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            LOG("Failed to set name '%s'. Error: %d\n", DebugNameInfo.pObjectName, Result);
        }
    }
}

FGraphicsPipeline* FGraphicsPipeline::Create(FDevice* pDevice, const FGraphicsPipelineStateParams& Params)
{
    FGraphicsPipeline* pPipeline = new FGraphicsPipeline(pDevice);
    assert(Params.pVertexShader != nullptr);
    assert(Params.pRenderPass != nullptr);
    assert(Params.pPipelineLayout != nullptr);
    
    std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;

    VkPipelineShaderStageCreateInfo ShaderStageCreateInfo;
    ZERO_STRUCT(&ShaderStageCreateInfo);
    
    ShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    if (Params.pVertexShader)
    {
        ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
        ShaderStageCreateInfo.module = Params.pVertexShader->GetModule();
        ShaderStageCreateInfo.pName  = Params.pVertexShader->GetEntryPoint();
        ShaderStages.push_back(ShaderStageCreateInfo);
    }

    if (Params.pFragmentShader)
    {
        ShaderStageCreateInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        ShaderStageCreateInfo.module = Params.pFragmentShader->GetModule();
        ShaderStageCreateInfo.pName  = Params.pFragmentShader->GetEntryPoint();
        ShaderStages.push_back(ShaderStageCreateInfo);
    }

    VkPipelineVertexInputStateCreateInfo VertexInputCreateInfo;
    ZERO_STRUCT(&VertexInputCreateInfo);
    
    VertexInputCreateInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VertexInputCreateInfo.vertexBindingDescriptionCount   = Params.BindingDescriptionCount;
    VertexInputCreateInfo.pVertexBindingDescriptions      = Params.pBindingDescriptions;
    VertexInputCreateInfo.vertexAttributeDescriptionCount = Params.AttributeDescriptionCount;
    VertexInputCreateInfo.pVertexAttributeDescriptions    = Params.pAttributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo InputAssemblyCreateInfo;
    ZERO_STRUCT(&InputAssemblyCreateInfo);
    
    InputAssemblyCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyCreateInfo.topology               = Params.Topology;
    InputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo ViewportStateCreateInfo;
    ZERO_STRUCT(&ViewportStateCreateInfo);
    
    ViewportStateCreateInfo.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportStateCreateInfo.viewportCount = 1;
    ViewportStateCreateInfo.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo RasterizerCreateInfo;
    ZERO_STRUCT(&RasterizerCreateInfo);
    
    RasterizerCreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    RasterizerCreateInfo.depthClampEnable        = VK_FALSE;
    RasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    RasterizerCreateInfo.polygonMode             = Params.PolygonMode;
    RasterizerCreateInfo.lineWidth               = 1.0f;
    RasterizerCreateInfo.cullMode                = Params.CullMode;
    RasterizerCreateInfo.frontFace               = Params.FrontFace;
    RasterizerCreateInfo.depthBiasEnable         = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo MultisamplingCreateInfo;
    ZERO_STRUCT(&MultisamplingCreateInfo);
    
    MultisamplingCreateInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    MultisamplingCreateInfo.sampleShadingEnable   = VK_FALSE;
    MultisamplingCreateInfo.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState ColorBlendAttachment;
    ZERO_STRUCT(&ColorBlendAttachment);
    
    if (!Params.bBlendEnable)
    {
        ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        ColorBlendAttachment.blendEnable    = VK_FALSE;
    }
    else
    {
        ColorBlendAttachment.blendEnable         = VK_TRUE;
        ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        ColorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
        ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        ColorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
        ColorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }

    VkPipelineColorBlendStateCreateInfo ColorBlendingCreateInfo;
    ZERO_STRUCT(&ColorBlendingCreateInfo);
    
    ColorBlendingCreateInfo.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlendingCreateInfo.logicOpEnable   = VK_FALSE;
    ColorBlendingCreateInfo.logicOp         = VK_LOGIC_OP_COPY;
    ColorBlendingCreateInfo.attachmentCount = 1;
    ColorBlendingCreateInfo.pAttachments    = &ColorBlendAttachment;

    VkDynamicState DynamicStates[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo DynamicStateInfoCreateInfo;
    ZERO_STRUCT(&DynamicStateInfoCreateInfo);
    
    DynamicStateInfoCreateInfo.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicStateInfoCreateInfo.pDynamicStates    = DynamicStates;
    DynamicStateInfoCreateInfo.dynamicStateCount = 2;

    assert(Params.pRenderPass != nullptr);

    VkPipelineDepthStencilStateCreateInfo DepthStencilCreateInfo;
    ZERO_STRUCT(&DepthStencilCreateInfo);
    
    DepthStencilCreateInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilCreateInfo.depthTestEnable       = VK_TRUE;
    DepthStencilCreateInfo.depthWriteEnable      = VK_TRUE;
    DepthStencilCreateInfo.depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
    DepthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
    DepthStencilCreateInfo.stencilTestEnable     = VK_FALSE;

    // Add the depth stencil state to the pipeline creation info
    VkGraphicsPipelineCreateInfo PipelineCreateInfo;
    ZERO_STRUCT(&PipelineCreateInfo);

    PipelineCreateInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineCreateInfo.stageCount          = uint32_t(ShaderStages.size());
    PipelineCreateInfo.pStages             = ShaderStages.data();
    PipelineCreateInfo.pVertexInputState   = &VertexInputCreateInfo;
    PipelineCreateInfo.pInputAssemblyState = &InputAssemblyCreateInfo;
    PipelineCreateInfo.pViewportState      = &ViewportStateCreateInfo;
    PipelineCreateInfo.pRasterizationState = &RasterizerCreateInfo;
    PipelineCreateInfo.pMultisampleState   = &MultisamplingCreateInfo;
    PipelineCreateInfo.pDepthStencilState  = Params.bDepthEnable ? &DepthStencilCreateInfo : nullptr;
    PipelineCreateInfo.pColorBlendState    = &ColorBlendingCreateInfo;
    PipelineCreateInfo.pDynamicState       = &DynamicStateInfoCreateInfo;
    PipelineCreateInfo.renderPass          = Params.pRenderPass->GetRenderPass();
    PipelineCreateInfo.layout              = Params.pPipelineLayout->GetPipelineLayout();
    PipelineCreateInfo.subpass             = 0;
    PipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;
    PipelineCreateInfo.basePipelineIndex   = -1;

    VkResult Result = vkCreateGraphicsPipelines(pDevice->GetDevice(), VK_NULL_HANDLE, 1, &PipelineCreateInfo, nullptr, &pPipeline->m_Pipeline);
    if (Result != VK_SUCCESS)
    {
        LOG("vkCreateGraphicsPipelines failed\n");
        return nullptr;
    }
    else
    {
        LOG("Created Graphics-Pipeline\n");
    }
    
    return pPipeline;
}

FGraphicsPipeline::FGraphicsPipeline(FDevice* pDevice)
    : FBasePipeline(pDevice)
{
}

FComputePipeline* FComputePipeline::Create(FDevice* pDevice, const FComputePipelineStateParams& Params)
{
    FComputePipeline* pPipeline = new FComputePipeline(pDevice);
    assert(Params.pShader != nullptr);
    assert(Params.pPipelineLayout != nullptr);

    VkPipelineShaderStageCreateInfo ShaderStageInfo;
    ZERO_STRUCT(&ShaderStageInfo);
    
    ShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    ShaderStageInfo.module = Params.pShader->GetModule();
    ShaderStageInfo.pName  = Params.pShader->GetEntryPoint();
    
    VkComputePipelineCreateInfo PipelineCreateInfo;
    ZERO_STRUCT(&PipelineCreateInfo);
    
    PipelineCreateInfo.sType             = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    PipelineCreateInfo.basePipelineIndex = -1;
    PipelineCreateInfo.layout            = Params.pPipelineLayout->GetPipelineLayout();
    PipelineCreateInfo.stage             = ShaderStageInfo;

    VkResult Result = vkCreateComputePipelines(pDevice->GetDevice(), VK_NULL_HANDLE, 1, &PipelineCreateInfo, nullptr, &pPipeline->m_Pipeline);
    if (Result != VK_SUCCESS)
    {
        LOG("vkCreateComputePipelines failed\n");
        return nullptr;
    }
    else
    {
        LOG("Created Compute-Pipeline\n");
    }
    
    return pPipeline;
}

FComputePipeline::FComputePipeline(FDevice* pDevice)
    : FBasePipeline(pDevice)
{
}

FRayTracingPipeline* FRayTracingPipeline::Create(class FDevice* pDevice, const FRayTracingPipelineStateParams& Params)
{
    FRayTracingPipeline* pPipeline = new FRayTracingPipeline(pDevice);

    std::vector<VkPipelineShaderStageCreateInfo>      ShaderStages;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> ShaderGroups;

    // Ray generation group
    {
        VkPipelineShaderStageCreateInfo ShaderStage = { };
        ShaderStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStage.stage  = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        ShaderStage.module = Params.pRayGenShader->GetModule();
        ShaderStage.pName  = Params.pRayGenShader->GetEntryPoint();
        ShaderStages.push_back(ShaderStage);

        VkRayTracingShaderGroupCreateInfoKHR ShaderGroup = { };
        ShaderGroup.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        ShaderGroup.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        ShaderGroup.generalShader      = static_cast<uint32_t>(ShaderStages.size()) - 1;
        ShaderGroup.closestHitShader   = VK_SHADER_UNUSED_KHR;
        ShaderGroup.anyHitShader       = VK_SHADER_UNUSED_KHR;
        ShaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
        ShaderGroups.push_back(ShaderGroup);
    }

    // Miss group
    {
        VkPipelineShaderStageCreateInfo ShaderStage = { };
        ShaderStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStage.stage  = VK_SHADER_STAGE_MISS_BIT_KHR;
        ShaderStage.module = Params.pRayMissShader->GetModule();
        ShaderStage.pName  = Params.pRayMissShader->GetEntryPoint();
        ShaderStages.push_back(ShaderStage);

        VkRayTracingShaderGroupCreateInfoKHR ShaderGroup = { };
        ShaderGroup.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        ShaderGroup.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        ShaderGroup.generalShader      = static_cast<uint32_t>(ShaderStages.size()) - 1;
        ShaderGroup.closestHitShader   = VK_SHADER_UNUSED_KHR;
        ShaderGroup.anyHitShader       = VK_SHADER_UNUSED_KHR;
        ShaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
        ShaderGroups.push_back(ShaderGroup);
    }

    // Closest hit group
    {
        VkPipelineShaderStageCreateInfo ShaderStage = { };
        ShaderStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStage.stage  = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        ShaderStage.module = Params.pRayClosestHitShader->GetModule();
        ShaderStage.pName  = Params.pRayClosestHitShader->GetEntryPoint();
        ShaderStages.push_back(ShaderStage);

        VkRayTracingShaderGroupCreateInfoKHR ShaderGroup = { };
        ShaderGroup.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        ShaderGroup.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        ShaderGroup.generalShader      = VK_SHADER_UNUSED_KHR;
        ShaderGroup.closestHitShader   = static_cast<uint32_t>(ShaderStages.size()) - 1;
        ShaderGroup.anyHitShader       = VK_SHADER_UNUSED_KHR;
        ShaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
        ShaderGroups.push_back(ShaderGroup);
    }

    VkRayTracingPipelineCreateInfoKHR PipelineCreateInfo;
    ZERO_STRUCT(&PipelineCreateInfo);

    PipelineCreateInfo.sType                        = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    PipelineCreateInfo.stageCount                   = static_cast<uint32_t>(ShaderStages.size());
    PipelineCreateInfo.pStages                      = ShaderStages.data();
    PipelineCreateInfo.groupCount                   = static_cast<uint32_t>(ShaderGroups.size());
    PipelineCreateInfo.pGroups                      = ShaderGroups.data();
    PipelineCreateInfo.maxPipelineRayRecursionDepth = Params.MaxPipelineRayRecursionDepth;
    PipelineCreateInfo.layout                       = Params.pPipelineLayout->GetPipelineLayout();

    VkResult Result = FExtensions::vkCreateRayTracingPipelinesKHR(pDevice->GetDevice(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &PipelineCreateInfo, nullptr, &pPipeline->m_Pipeline);
    if (Result != VK_SUCCESS)
    {
        LOG("vkCreateRayTracingPipelinesKHR failed\n");
        return nullptr;
    }
    else
    {
        LOG("Created Graphics-Pipeline\n");
    }

    return pPipeline;
}

FRayTracingPipeline::FRayTracingPipeline(FDevice* pDevice)
    : FBasePipeline(pDevice)
{
}