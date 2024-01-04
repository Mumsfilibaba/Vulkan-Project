#include "PipelineState.h"
#include "ShaderModule.h"
#include "RenderPass.h"
#include "Device.h"
#include "PipelineLayout.h"
#include <vector>

BasePipeline::BasePipeline(VkDevice device)
    : m_Device(device)
    , m_Pipeline(VK_NULL_HANDLE)
{
}

BasePipeline::~BasePipeline()
{
    if (m_Pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
        m_Pipeline = VK_NULL_HANDLE;
    }
    
    m_Device = VK_NULL_HANDLE;
}

GraphicsPipeline* GraphicsPipeline::Create(Device* pDevice, const GraphicsPipelineStateParams& params)
{
    GraphicsPipeline* pPipeline = new GraphicsPipeline(pDevice->GetDevice());
    assert(params.pVertexShader != nullptr);
    assert(params.pRenderPass != nullptr);
    assert(params.pPipelineLayout != nullptr);
    
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    VkPipelineShaderStageCreateInfo shaderStageInfo;
    ZERO_STRUCT(&shaderStageInfo);
    
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    if (params.pVertexShader)
    {
        shaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStageInfo.module = params.pVertexShader->GetModule();
        shaderStageInfo.pName  = params.pVertexShader->GetEntryPoint();
        shaderStages.push_back(shaderStageInfo);
    }

    if (params.pFragmentShader)
    {
        shaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStageInfo.module = params.pFragmentShader->GetModule();
        shaderStageInfo.pName  = params.pFragmentShader->GetEntryPoint();
        shaderStages.push_back(shaderStageInfo);
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    ZERO_STRUCT(&vertexInputInfo);
    
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = params.bindingDescriptionCount;
    vertexInputInfo.pVertexBindingDescriptions      = params.pBindingDescriptions;
    vertexInputInfo.vertexAttributeDescriptionCount = params.attributeDescriptionCount;
    vertexInputInfo.pVertexAttributeDescriptions    = params.pAttributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    ZERO_STRUCT(&inputAssembly);
    
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState;
    ZERO_STRUCT(&viewportState);
    
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer;
    ZERO_STRUCT(&rasterizer);
    
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = params.cullMode;
    rasterizer.frontFace               = params.frontFace;
    rasterizer.depthBiasEnable         = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling;
    ZERO_STRUCT(&multisampling);
    
    multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable   = VK_FALSE;
    multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    ZERO_STRUCT(&colorBlendAttachment);
    
    if (!params.bBlendEnable)
    {
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable    = VK_FALSE;
    }
    else
    {
        colorBlendAttachment.blendEnable         = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
        colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending;
    ZERO_STRUCT(&colorBlending);
    
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.logicOp         = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;

    VkDynamicState dynamicStates[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState;
    ZERO_STRUCT(&dynamicState);
    
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates    = dynamicStates;
    dynamicState.dynamicStateCount = 2;

    assert(params.pRenderPass != nullptr);

    VkGraphicsPipelineCreateInfo pipelineInfo;
    ZERO_STRUCT(&pipelineInfo);
    
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount          = uint32_t(shaderStages.size());
    pipelineInfo.pStages             = shaderStages.data();
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = nullptr;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.renderPass          = params.pRenderPass->GetRenderPass();
    pipelineInfo.layout              = params.pPipelineLayout->GetPipelineLayout();
    pipelineInfo.subpass             = 0;
    pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex   = -1;

    VkResult result = vkCreateGraphicsPipelines(pPipeline->m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pPipeline->m_Pipeline);
    if (result != VK_SUCCESS) 
    {
        std::cout << "vkCreateGraphicsPipelines failed\n";
        return nullptr;
    }
    else
    {
        std::cout << "Created Graphics-Pipeline\n";
    }
    
    return pPipeline;
}

GraphicsPipeline::GraphicsPipeline(VkDevice device)
    : BasePipeline(device)
{
}


ComputePipeline* ComputePipeline::Create(Device* pDevice, const ComputePipelineStateParams& params)
{
    ComputePipeline* newPipeline = new ComputePipeline(pDevice->GetDevice());
    assert(params.pShader != nullptr);
    assert(params.pPipelineLayout != nullptr);

    VkPipelineShaderStageCreateInfo shaderStageInfo;
    ZERO_STRUCT(&shaderStageInfo);
    
    shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageInfo.module = params.pShader->GetModule();
    shaderStageInfo.pName  = params.pShader->GetEntryPoint();
    
    VkComputePipelineCreateInfo pipelineInfo;
    ZERO_STRUCT(&pipelineInfo);
    
    pipelineInfo.sType             = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.layout            = params.pPipelineLayout->GetPipelineLayout();
    pipelineInfo.stage             = shaderStageInfo;

    VkResult result = vkCreateComputePipelines(newPipeline->m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline->m_Pipeline);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkCreateComputePipelines failed\n";
        return nullptr;
    }
    else
    {
        std::cout << "Created Compute-Pipeline\n";
    }
    
    return newPipeline;
}

ComputePipeline::ComputePipeline(VkDevice device)
    : BasePipeline(device)
{
}
