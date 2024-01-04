#include "DescriptorSetLayout.h"
#include "Device.h"

DescriptorSetLayout* DescriptorSetLayout::Create(Device* pDevice, const DescriptorSetLayoutParams& params)
{
    DescriptorSetLayout* pDescriptorSetLayout = new DescriptorSetLayout(pDevice->GetDevice());
    
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

DescriptorSetLayout::DescriptorSetLayout(VkDevice device)
    : m_Device(device)
    , m_DescriptorSetLayout(VK_NULL_HANDLE)
{
}

DescriptorSetLayout::~DescriptorSetLayout()
{
    if (m_DescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
        m_DescriptorSetLayout = VK_NULL_HANDLE;
    }

    m_Device = VK_NULL_HANDLE;
}
