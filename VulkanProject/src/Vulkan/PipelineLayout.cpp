#include "PipelineLayout.h"
#include "Device.h"
#include "DescriptorSetLayout.h"
#include "BindlessManager.h"

FPipelineLayout* FPipelineLayout::Create(FDevice* pDevice, const FPipelineLayoutParams& Params)
{
    FPipelineLayout* pPipelineLayout = new FPipelineLayout(pDevice);

    std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
    DescriptorSetLayouts.reserve(Params.NumLayouts);

    for (uint32_t i = 0; i < Params.NumLayouts; i++)
    {
        DescriptorSetLayouts.push_back(Params.ppLayouts[i]->GetDescriptorSetLayout());
    }
    
    if (Params.bEnableBindless)
    {
        pPipelineLayout->m_BindlessDescriptorSetIndex = static_cast<uint32_t>(DescriptorSetLayouts.size());
        DescriptorSetLayouts.push_back(pDevice->GetBindlessManager().GetDescriptorSetLayout());
    }

    VkPushConstantRange PushConstantRanges[1] = {};
    PushConstantRanges[0].stageFlags = VK_SHADER_STAGE_ALL;
    PushConstantRanges[0].offset     = sizeof(uint32_t) * 0;
    PushConstantRanges[0].size       = sizeof(uint32_t) * Params.NumPushConstants;
    
    VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo;
    ZERO_STRUCT(&PipelineLayoutCreateInfo);
    
    PipelineLayoutCreateInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineLayoutCreateInfo.setLayoutCount = DescriptorSetLayouts.size();
    PipelineLayoutCreateInfo.pSetLayouts    = DescriptorSetLayouts.data();
    
    if (Params.NumPushConstants > 0)
    {
        PipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        PipelineLayoutCreateInfo.pPushConstantRanges    = PushConstantRanges;
    }
    else
    {
        PipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        PipelineLayoutCreateInfo.pPushConstantRanges    = nullptr;
    }

    VkResult Result = vkCreatePipelineLayout(pDevice->GetDevice(), &PipelineLayoutCreateInfo, nullptr, &pPipelineLayout->m_PipelineLayout);
    if (Result != VK_SUCCESS)
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

FPipelineLayout::FPipelineLayout(FDevice* pDevice)
    : FDeviceChild(pDevice)
    , m_PipelineLayout(VK_NULL_HANDLE)
    , m_BindlessDescriptorSetIndex(uint32_t(-1))
{
}

FPipelineLayout::~FPipelineLayout()
{
    if (m_PipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(GetDevice()->GetDevice(), m_PipelineLayout, nullptr);
        m_PipelineLayout = VK_NULL_HANDLE;
    }
}
