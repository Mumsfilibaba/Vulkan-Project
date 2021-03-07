#include "PipelineState.h"
#include "ShaderModule.h"
#include "RenderPass.h"
#include "VulkanContext.h"

#include <vector>

BasePipeline::BasePipeline(VkDevice device)
	: m_Device(device)
	, m_Pipeline(VK_NULL_HANDLE)
	, m_Layout(VK_NULL_HANDLE)
	, m_DescriptorSetLayout(VK_NULL_HANDLE)
{
}

BasePipeline::~BasePipeline()
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
	
	if (m_DescriptorSetLayout != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
		m_DescriptorSetLayout = nullptr;
		
		std::cout << "Destroyed PipelineLayout" << std::endl;
	}
	
	m_Device = VK_NULL_HANDLE;
}


GraphicsPipeline::GraphicsPipeline(VkDevice device)
	: BasePipeline(device)
{
}

GraphicsPipeline* GraphicsPipeline::Create(VulkanContext* pContext, const GraphicsPipelineStateParams& params)
{
	GraphicsPipeline* newPipeline = new GraphicsPipeline(pContext->GetDevice());
	
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
        VkResult result = vkCreatePipelineLayout(newPipeline->m_Device, &pipelineLayoutInfo, nullptr, &newPipeline->m_Layout);
        if (result != VK_SUCCESS) 
        {
            std::cout << "vkCreatePipelineLayout failed" << std::endl;
            return nullptr;
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
    pipelineInfo.layout     = newPipeline->m_Layout;
    pipelineInfo.subpass    = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex  = -1;

    VkResult result = vkCreateGraphicsPipelines(newPipeline->m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline->m_Pipeline);
    if (result != VK_SUCCESS) 
    {
        std::cout << "vkCreateGraphicsPipelines failed" << std::endl;
		return nullptr;
    }
    else
    {
        std::cout << "Created Graphics-Pipeline" << std::endl;
    }
	
	return newPipeline;
}

ComputePipeline::ComputePipeline(VkDevice device)
	: BasePipeline(device)
{
}

ComputePipeline* ComputePipeline::Create(VulkanContext* pContext, const ComputePipelineStateParams& params)
{
	ComputePipeline* newPipeline = new ComputePipeline(pContext->GetDevice());

	VkDescriptorSetLayoutBinding imageLayoutBinding = {};
	imageLayoutBinding.binding 		      = 0;
	imageLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	imageLayoutBinding.descriptorCount    = 1;
	imageLayoutBinding.stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
	imageLayoutBinding.pImmutableSamplers = nullptr;
	
	VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo = {};
	descriptorLayoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayoutInfo.flags        = 0;
	descriptorLayoutInfo.bindingCount = 1;
	descriptorLayoutInfo.pBindings    = &imageLayoutBinding;
	
	if (vkCreateDescriptorSetLayout(newPipeline->m_Device, &descriptorLayoutInfo, nullptr, &newPipeline->m_DescriptorSetLayout) != VK_SUCCESS)
	{
		std::cout << "vkCreatePipelineLayout failed" << std::endl;
		return nullptr;
	}
	
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount         = 1;
	pipelineLayoutInfo.pSetLayouts 		      = &newPipeline->m_DescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges    = nullptr;
	{
		VkResult result = vkCreatePipelineLayout(newPipeline->m_Device, &pipelineLayoutInfo, nullptr, &newPipeline->m_Layout);
		if (result != VK_SUCCESS)
		{
			std::cout << "vkCreatePipelineLayout failed" << std::endl;
			return nullptr;
		}
		else
		{
			std::cout << "Created PipelineLayout" << std::endl;
		}
	}

	VkPipelineShaderStageCreateInfo shaderStageInfo = {};
	shaderStageInfo.sType   = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfo.pNext   = nullptr;
	shaderStageInfo.flags   = 0;
	shaderStageInfo.pSpecializationInfo = nullptr;
	
	assert(params.pShader);

	shaderStageInfo.stage   = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStageInfo.module  = params.pShader->GetModule();
	shaderStageInfo.pName   = params.pShader->GetEntryPoint();
	
	VkComputePipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType      	    = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex  = -1;
	pipelineInfo.flags 				= 0;
	pipelineInfo.pNext 				= nullptr;
	pipelineInfo.layout 			= newPipeline->m_Layout;
	pipelineInfo.stage				= shaderStageInfo;

	VkResult result = vkCreateComputePipelines(newPipeline->m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline->m_Pipeline);
	if (result != VK_SUCCESS)
	{
		std::cout << "vkCreateComputePipelines failed" << std::endl;
		return nullptr;
	}
	else
	{
		std::cout << "Created Compute-Pipeline" << std::endl;
	}
	
	return newPipeline;
}
