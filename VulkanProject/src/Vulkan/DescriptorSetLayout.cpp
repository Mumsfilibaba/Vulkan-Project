#include "DescriptorSetLayout.h"
#include "Device.h"

FDescriptorSetLayout* FDescriptorSetLayout::Create(FDevice* pDevice, const FDescriptorSetLayoutParams& params)
{
    FDescriptorSetLayout* pDescriptorSetLayout = new FDescriptorSetLayout(pDevice->GetDevice());
    
    VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo;
    ZERO_STRUCT(&descriptorLayoutInfo);
    
    descriptorLayoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayoutInfo.bindingCount = params.numBindings;
    descriptorLayoutInfo.pBindings    = params.pBindings;

    VkResult result = vkCreateDescriptorSetLayout(pDevice->GetDevice(), &descriptorLayoutInfo, nullptr, &pDescriptorSetLayout->m_DescriptorSetLayout);
    if (result != VK_SUCCESS)
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

FDescriptorSetLayout::FDescriptorSetLayout(VkDevice device)
    : m_Device(device)
    , m_DescriptorSetLayout(VK_NULL_HANDLE)
{
}

FDescriptorSetLayout::~FDescriptorSetLayout()
{
    if (m_DescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
        m_DescriptorSetLayout = VK_NULL_HANDLE;
    }

    m_Device = VK_NULL_HANDLE;
}
