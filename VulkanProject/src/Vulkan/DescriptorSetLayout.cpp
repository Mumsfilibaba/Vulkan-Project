#include "DescriptorSetLayout.h"
#include "Device.h"
#include "Extensions.h"

CDescriptorSetLayout* CDescriptorSetLayout::Create(CDevice* pDevice, const SDescriptorSetLayoutParams& Params)
{
    CDescriptorSetLayout* pDescriptorSetLayout = new CDescriptorSetLayout(pDevice);
    
    VkDescriptorSetLayoutCreateInfo DescriptorLayoutCreateInfo;
    ZERO_STRUCT(&DescriptorLayoutCreateInfo);
    
    DescriptorLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    DescriptorLayoutCreateInfo.bindingCount = Params.NumBindings;
    DescriptorLayoutCreateInfo.pBindings    = Params.pBindings;

    VkResult Result = vkCreateDescriptorSetLayout(pDevice->GetDevice(), &DescriptorLayoutCreateInfo, nullptr, &pDescriptorSetLayout->m_DescriptorSetLayout);
    if (Result != VK_SUCCESS)
    {
        LOG("vkCreatePipelineLayout failed\n");
        return nullptr;
    }
    else
    {
        LOG("Created DescriptorSetLayout\n");
        return pDescriptorSetLayout;
    }
}

CDescriptorSetLayout::CDescriptorSetLayout(CDevice* pDevice)
    : CDeviceChild(pDevice)
    , m_DescriptorSetLayout(VK_NULL_HANDLE)
{
}

CDescriptorSetLayout::~CDescriptorSetLayout()
{
    if (m_DescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(GetDevice()->GetDevice(), m_DescriptorSetLayout, nullptr);
        m_DescriptorSetLayout = VK_NULL_HANDLE;
    }
}

void CDescriptorSetLayout::SetDebugName(const char* DebugName)
{
    if (Extensions::vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT DebugNameInfo;
        ZERO_STRUCT(&DebugNameInfo);

        DebugNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        DebugNameInfo.objectType   = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
        DebugNameInfo.pObjectName  = DebugName;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_DescriptorSetLayout);

        VkResult Result = Extensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            LOG("Failed to set name '%s'. Error: %d\n", DebugNameInfo.pObjectName, Result);
        }
    }
}