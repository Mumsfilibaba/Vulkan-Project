#include "BindlessManager.h"
#include "Device.h"
#include "DescriptorPool.h"
#include "DescriptorSet.h"
#include "TextureView.h"

CBindlessManager* CBindlessManager::Create(CDevice* pDevice)
{
    CBindlessManager* pBindlessManager = new CBindlessManager(pDevice);

    const VkPhysicalDeviceLimits& DeviceLimits = pDevice->GetDeviceLimits();
    if (DeviceLimits.maxPerStageDescriptorSampledImages < 16)
    {
        LOG("Not enough samplers/images supported for bindless\n");
        return nullptr;
    }

    const size_t NumMaxBindlessResources      = DeviceLimits.maxPerStageDescriptorSampledImages - 16;
    const size_t BindlessResourceBindingIndex = 0;
    pBindlessManager->m_MaxTextureBinding = static_cast<uint32_t>(NumMaxBindlessResources);
    
    // DescriptorPool
    constexpr uint32_t NumPoolSizes = 1;
    VkDescriptorPoolSize PoolSizes[NumPoolSizes];
    PoolSizes[0].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    PoolSizes[0].descriptorCount = NumMaxBindlessResources;
    
    VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo;
    ZERO_STRUCT(&DescriptorPoolCreateInfo);
    
    DescriptorPoolCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    DescriptorPoolCreateInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
    DescriptorPoolCreateInfo.poolSizeCount = NumPoolSizes;
    DescriptorPoolCreateInfo.pPoolSizes    = PoolSizes;
    DescriptorPoolCreateInfo.maxSets       = 1;
    
    VkResult Result = vkCreateDescriptorPool(pDevice->GetDevice(), &DescriptorPoolCreateInfo, nullptr, &pBindlessManager->m_DescriptorPool);
    if (Result != VK_SUCCESS)
    {
        LOG("Bindless vkCreateDescriptorPool failed\n");
        return nullptr;
    }
    else
    {
        LOG("Created Bindless DescriptorPool\n");
    }
    
    // DescriptorSetLayout
    constexpr uint32_t NumBindings = 1;
    VkDescriptorSetLayoutBinding Bindings[NumBindings];
    
    Bindings[0].binding            = BindlessResourceBindingIndex;
    Bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    Bindings[0].descriptorCount    = NumMaxBindlessResources;
    Bindings[0].stageFlags         = VK_SHADER_STAGE_ALL;
    Bindings[0].pImmutableSamplers = nullptr;
    
    VkDescriptorSetLayoutCreateInfo DescriptorLayoutCreateInfo;
    ZERO_STRUCT(&DescriptorLayoutCreateInfo);
    
    DescriptorLayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    DescriptorLayoutCreateInfo.flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
    DescriptorLayoutCreateInfo.bindingCount = NumBindings;
    DescriptorLayoutCreateInfo.pBindings    = Bindings;

    VkDescriptorBindingFlags BindlessFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
    
    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT BindingFlagsCreateInfo;
    ZERO_STRUCT(&BindingFlagsCreateInfo);
    
    BindingFlagsCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    BindingFlagsCreateInfo.bindingCount  = 1;
    BindingFlagsCreateInfo.pBindingFlags = &BindlessFlags;

    DescriptorLayoutCreateInfo.pNext = &BindingFlagsCreateInfo;
    
    Result = vkCreateDescriptorSetLayout(pDevice->GetDevice(), &DescriptorLayoutCreateInfo, nullptr, &pBindlessManager->m_DescriptorSetLayout);
    if (Result != VK_SUCCESS)
    {
        LOG("Bindless vkCreatePipelineLayout failed\n");
        return nullptr;
    }
    else
    {
        LOG("Created Bindless DescriptorSetLayout\n");
    }
    
    VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo;
    ZERO_STRUCT(&DescriptorSetAllocateInfo);

    DescriptorSetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DescriptorSetAllocateInfo.descriptorSetCount = 1;
    DescriptorSetAllocateInfo.pSetLayouts        = &pBindlessManager->m_DescriptorSetLayout;
    DescriptorSetAllocateInfo.descriptorPool     = pBindlessManager->m_DescriptorPool;
    
    uint32_t MaxBinding = NumMaxBindlessResources - 1;
    VkDescriptorSetVariableDescriptorCountAllocateInfoEXT DescriptorCountAllocateInfo;
    ZERO_STRUCT(&DescriptorCountAllocateInfo);
    
    DescriptorCountAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
    DescriptorCountAllocateInfo.descriptorSetCount = 1;
    DescriptorCountAllocateInfo.pDescriptorCounts  = &MaxBinding;
    DescriptorSetAllocateInfo.pNext = &DescriptorCountAllocateInfo;

    Result = vkAllocateDescriptorSets(pDevice->GetDevice(), &DescriptorSetAllocateInfo, &pBindlessManager->m_DescriptorSet);
    if (Result != VK_SUCCESS)
    {
        LOG("Bindless vkAllocateDescriptorSets failed\n");
        return nullptr;
    }
    else
    {
        LOG("Allocated Bindless DescriptorSet\n");
    }
    
    return pBindlessManager;
}

CBindlessManager::CBindlessManager(CDevice* pDevice)
    : CDeviceChild(pDevice)
    , m_DescriptorPool(VK_NULL_HANDLE)
    , m_DescriptorSet(VK_NULL_HANDLE)
    , m_DescriptorSetLayout(VK_NULL_HANDLE)
    , m_NextTextureBinding(0)
    , m_MaxTextureBinding()
{
}

CBindlessManager::~CBindlessManager()
{
    if (m_DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(GetDevice()->GetDevice(), m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }
    
    if (m_DescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(GetDevice()->GetDevice(), m_DescriptorSetLayout, nullptr);
        m_DescriptorSetLayout = VK_NULL_HANDLE;
    }
}

uint32_t CBindlessManager::AddImageView(VkImageView ImageView, VkSampler Sampler)
{
    assert(m_DescriptorSet != VK_NULL_HANDLE);
    assert(Sampler != VK_NULL_HANDLE);
    assert(ImageView != VK_NULL_HANDLE);
    
    // Generate a bindless ID
    uint32_t BindlessHandle = InvalidBindlessID;
    if (m_TextureFreeList.empty())
    {
        assert(m_NextTextureBinding < m_MaxTextureBinding);
        
        if (m_NextTextureBinding < m_MaxTextureBinding)
        {
            BindlessHandle = m_NextTextureBinding++;
        }
    }
    else
    {
        BindlessHandle = m_TextureFreeList.back();
        m_TextureFreeList.pop_back();
    }
    
    if (BindlessHandle != InvalidBindlessID)
    {
        m_TextureBindings.insert(std::make_pair(ImageView, BindlessHandle));
    }
    else
    {
        return InvalidBindlessID;
    }

    // Write Descriptor
    VkDescriptorImageInfo ImageInfo = {};
    ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ImageInfo.imageView   = ImageView;
    ImageInfo.sampler     = Sampler;
    
    VkWriteDescriptorSet DescriptorWrite = {};
    ZERO_STRUCT(&DescriptorWrite);

    DescriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DescriptorWrite.dstSet           = m_DescriptorSet;
    DescriptorWrite.dstBinding       = 0;
    DescriptorWrite.dstArrayElement  = BindlessHandle;
    DescriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    DescriptorWrite.descriptorCount  = 1;
    DescriptorWrite.pBufferInfo      = nullptr;
    DescriptorWrite.pImageInfo       = &ImageInfo;
    DescriptorWrite.pTexelBufferView = nullptr;
    
    vkUpdateDescriptorSets(GetDevice()->GetDevice(), 1, &DescriptorWrite, 0, nullptr);
    return BindlessHandle;
}

void CBindlessManager::RemoveImageView(VkImageView ImageView)
{
    auto It = m_TextureBindings.find(ImageView);
    if (It == m_TextureBindings.end())
    {
        return;
    }

    m_TextureFreeList.emplace_back(It->second);
    m_TextureBindings.erase(It);
}
