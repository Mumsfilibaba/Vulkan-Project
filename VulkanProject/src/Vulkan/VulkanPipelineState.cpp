#include "VulkanPipelineState.h"
#include "VulkanShaderModule.h"
#include "VulkanRenderPass.h"

#include <vector>

VulkanGraphicsPipelineState::VulkanGraphicsPipelineState(VkDevice device, const GraphicsPipelineStateParams& params)
	: m_Device(device),
	m_Pipeline(VK_NULL_HANDLE),
	m_Layout(VK_NULL_HANDLE)
{
	Init(params);
}

VulkanGraphicsPipelineState::~VulkanGraphicsPipelineState()
{
	if (m_Pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
		m_Pipeline = VK_NULL_HANDLE;

        std::cout << "Destroyed Pipeline" << std::endl;
	}

	if (m_Layout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(m_Device, m_Layout, nullptr);
		m_Layout = VK_NULL_HANDLE;

        std::cout << "Destroyed PipelineLayout" << std::endl;
	}
}

void VulkanGraphicsPipelineState::Init(const GraphicsPipelineStateParams& params)
{
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    VkPipelineShaderStageCreateInfo shaderStageInfo = {};
    shaderStageInfo.sType   = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.pNext   = nullptr;
    shaderStageInfo.flags   = 0;
    shaderStageInfo.pSpecializationInfo = nullptr;
    
    if (params.pVertex)
    {
        shaderStageInfo.stage   = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStageInfo.module  = params.pVertex->GetModule();
        shaderStageInfo.pName   = params.pVertex->GetEntryPoint();
        shaderStages.push_back(shaderStageInfo);
    }

    if (params.pFragment)
    {
        shaderStageInfo.stage   = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStageInfo.module  = params.pFragment->GetModule();
        shaderStageInfo.pName   = params.pFragment->GetEntryPoint();
        shaderStages.push_back(shaderStageInfo);
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.flags = 0;
    vertexInputInfo.pNext = nullptr;
    vertexInputInfo.vertexBindingDescriptionCount   = params.BindingDescriptionCount;
    vertexInputInfo.pVertexBindingDescriptions      = params.pBindingDescriptions;
    vertexInputInfo.vertexAttributeDescriptionCount = params.AttributeDescriptionCount;
    vertexInputInfo.pVertexAttributeDescriptions    = params.pAttributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType     = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology  = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType 		= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = nullptr;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = nullptr;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable         = VK_FALSE;
    rasterizer.rasterizerDiscardEnable  = VK_FALSE;
    rasterizer.polygonMode  = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth    = 1.0f;
    rasterizer.cullMode     = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace    = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable   = VK_FALSE;
    multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable    = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable     = VK_FALSE;
    colorBlending.logicOp           = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount   = 1;
    colorBlending.pAttachments      = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    {
        VkResult result = vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_Layout);
        if (result != VK_SUCCESS) 
        {
            std::cout << "vkCreatePipelineLayout failed" << std::endl;
            return;
        }
        else
        {
            std::cout << "Created PipelineLayout" << std::endl;
        }
    }

    VkDynamicState dynamicStates[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.flags = 0;
    dynamicState.pNext = nullptr;
    dynamicState.pDynamicStates     = dynamicStates;
    dynamicState.dynamicStateCount  = 2;


    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = uint32_t(shaderStages.size());
    pipelineInfo.pStages    = shaderStages.data();
    pipelineInfo.pVertexInputState      = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState    = &inputAssembly;
    pipelineInfo.pViewportState         = &viewportState;
    pipelineInfo.pRasterizationState    = &rasterizer;
    pipelineInfo.pMultisampleState      = &multisampling;
    pipelineInfo.pDepthStencilState     = nullptr;
    pipelineInfo.pColorBlendState       = &colorBlending;
    pipelineInfo.pDynamicState          = &dynamicState;

    assert(params.pRenderPass != nullptr);

    pipelineInfo.renderPass = params.pRenderPass->GetRenderPass();
    pipelineInfo.layout     = m_Layout;
    pipelineInfo.subpass    = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex  = -1;

    VkResult result = vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline);
    if (result != VK_SUCCESS) 
    {
        std::cout << "vkCreateGraphicsPipelines failed" << std::endl;
    }
    else
    {
        std::cout << "Created Graphics-Pipeline" << std::endl;
    }
}
