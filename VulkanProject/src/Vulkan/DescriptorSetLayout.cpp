#include "DescriptorSetLayout.h"
#include "Device.h"
#include "Extensions.h"

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

void FDescriptorSetLayout::SetDebugName(const char* DebugName)
{
    if (FExtensions::vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT DebugNameInfo;
        ZERO_STRUCT(&DebugNameInfo);

        DebugNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        DebugNameInfo.objectType   = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
        DebugNameInfo.pObjectName  = DebugName;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_DescriptorSetLayout);

        VkResult Result = FExtensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            std::cout << "Failed to set name '" << DebugNameInfo.pObjectName << "'.Error: " << Result << std::endl;
        }
    }
}