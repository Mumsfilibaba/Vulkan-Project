#include "PipelineLayout.h"
#include "Device.h"
#include "DescriptorSetLayout.h"

FPipelineLayout* FPipelineLayout::Create(FDevice* pDevice, const FPipelineLayoutParams& params)
{
    FPipelineLayout* pPipelineLayout = new FPipelineLayout(pDevice->GetDevice());

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    descriptorSetLayouts.reserve(params.numLayouts);

    for (uint32_t i = 0; i < params.numLayouts; i++)
    {
        descriptorSetLayouts.push_back(params.ppLayouts[i]->GetDescriptorSetLayout());
    }

    VkPushConstantRange pushConstants[1] = {};
    pushConstants[0].stageFlags = VK_SHADER_STAGE_ALL;
    pushConstants[0].offset     = sizeof(uint32_t) * 0;
    pushConstants[0].size       = sizeof(uint32_t) * params.numPushConstants;
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    ZERO_STRUCT(&pipelineLayoutInfo);
    
    pipelineLayoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
    pipelineLayoutInfo.pSetLayouts    = descriptorSetLayouts.data();
    
    if (params.numPushConstants > 0)
    {
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges    = pushConstants;
    }
    else
    {
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges    = nullptr;
    }

    VkResult result = vkCreatePipelineLayout(pDevice->GetDevice(), &pipelineLayoutInfo, nullptr, &pPipelineLayout->m_PipelineLayout);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkCreatePipelineLayout failed\n";
        return nullptr;
    }
    else
    {
        std::cout << "Created PipelineLayout\n";
        return pPipelineLayout;
    }
}

FPipelineLayout::FPipelineLayout(VkDevice device)
    : m_Device(device)
    , m_PipelineLayout(VK_NULL_HANDLE)
{
}

FPipelineLayout::~FPipelineLayout()
{
    if (m_PipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
        m_PipelineLayout = VK_NULL_HANDLE;
    }

    m_Device = VK_NULL_HANDLE;
}
