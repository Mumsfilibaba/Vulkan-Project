#include "DescriptorSetLayout.h"
#include "Device.h"

FDescriptorSetLayout* FDescriptorSetLayout::Create(FDevice* pDevice, const FDescriptorSetLayoutParams& Params)
{
    FDescriptorSetLayout* pDescriptorSetLayout = new FDescriptorSetLayout(pDevice);
    
    VkDescriptorSetLayoutCreateInfo DescriptorLayoutCreateInfo;
    ZERO_STRUCT(&DescriptorLayoutCreateInfo);
    
    DescriptorLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    DescriptorLayoutCreateInfo.bindingCount = Params.NumBindings;
    DescriptorLayoutCreateInfo.pBindings    = Params.pBindings;

    VkResult Result = vkCreateDescriptorSetLayout(pDevice->GetDevice(), &DescriptorLayoutCreateInfo, nullptr, &pDescriptorSetLayout->m_DescriptorSetLayout);
    if (Result != VK_SUCCESS)
    {
        std::cout << "vkCreatePipelineLayout failed\n";
        return nullptr;
    }
    else
    {
        std::cout << "Created DescriptorSetLayout\n";
        return pDescriptorSetLayout;
    }
}

FDescriptorSetLayout::FDescriptorSetLayout(FDevice* pDevice)
    : FDeviceChild(pDevice)
    , m_DescriptorSetLayout(VK_NULL_HANDLE)
{
}

FDescriptorSetLayout::~FDescriptorSetLayout()
{
    if (m_DescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(GetDevice()->GetDevice(), m_DescriptorSetLayout, nullptr);
        m_DescriptorSetLayout = VK_NULL_HANDLE;
    }
}
