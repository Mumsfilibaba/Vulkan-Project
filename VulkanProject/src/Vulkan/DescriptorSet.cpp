#include "DescriptorSet.h"
#include "Device.h"
#include "DescriptorPool.h"
#include "PipelineState.h"
#include "DescriptorSetLayout.h"
#include "DescriptorSet.h"

// Allocates from pDescriptorPool and uses the layout from pPipeline
FDescriptorSet* FDescriptorSet::Create(FDevice* pDevice, FDescriptorPool* pDescriptorPool, FDescriptorSetLayout* pDescriptorSetLayout)
{
    assert(pDevice != nullptr);
    assert(pDescriptorPool != nullptr);
    assert(pDescriptorSetLayout != nullptr);
    
    FDescriptorSet* pDescriptorSet = new FDescriptorSet(pDevice, pDescriptorPool);
    
    VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo;
    ZERO_STRUCT(&DescriptorSetAllocateInfo);
    
    VkDescriptorSetLayout DescriptorLayout = pDescriptorSetLayout->GetDescriptorSetLayout();
    DescriptorSetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DescriptorSetAllocateInfo.descriptorSetCount = 1;
    DescriptorSetAllocateInfo.pSetLayouts        = &DescriptorLayout;
    DescriptorSetAllocateInfo.descriptorPool     = pDescriptorPool->GetPool();
    
    VkResult Result = vkAllocateDescriptorSets(pDevice->GetDevice(), &DescriptorSetAllocateInfo, &pDescriptorSet->m_DescriptorSet);
    if (Result != VK_SUCCESS)
    {
        std::cout << "vkAllocateDescriptorSets failed\n";
        return nullptr;
    }
    
    return pDescriptorSet;
}

FDescriptorSet::FDescriptorSet(FDevice* pDevice, FDescriptorPool* pDescriptorPool)
    : FDeviceChild(pDevice)
    , m_pDescriptorPool(pDescriptorPool)
    , m_DescriptorSet(VK_NULL_HANDLE)
{
    assert(m_pDescriptorPool != nullptr);
}

FDescriptorSet::~FDescriptorSet()
{
    if (m_DescriptorSet)
    {
        vkFreeDescriptorSets(GetDevice()->GetDevice(), m_pDescriptorPool->GetPool(), 1, &m_DescriptorSet);
        m_DescriptorSet = VK_NULL_HANDLE;
    }
}

void FDescriptorSet::BindStorageImage(VkImageView ImageView, uint32_t Binding)
{
    assert(m_DescriptorSet != VK_NULL_HANDLE);
    assert(ImageView != VK_NULL_HANDLE);
    
    VkDescriptorImageInfo ImageInfo = {};
    ImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    ImageInfo.imageView   = ImageView;
    ImageInfo.sampler     = VK_NULL_HANDLE;
    
    VkWriteDescriptorSet DescriptorWrite = {};
    ZERO_STRUCT(&DescriptorWrite);

    DescriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DescriptorWrite.dstSet           = m_DescriptorSet;
    DescriptorWrite.dstBinding       = Binding;
    DescriptorWrite.dstArrayElement  = 0;
    DescriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    DescriptorWrite.descriptorCount  = 1;
    DescriptorWrite.pBufferInfo      = nullptr;
    DescriptorWrite.pImageInfo       = &ImageInfo;
    DescriptorWrite.pTexelBufferView = nullptr;
    
    vkUpdateDescriptorSets(GetDevice()->GetDevice(), 1, &DescriptorWrite, 0, nullptr);
}

void FDescriptorSet::BindCombinedImageSampler(VkImageView ImageView, VkSampler Sampler, uint32_t Binding)
{
    assert(m_DescriptorSet != VK_NULL_HANDLE);
    assert(Sampler != VK_NULL_HANDLE);
    assert(ImageView != VK_NULL_HANDLE);
    
    VkDescriptorImageInfo ImageInfo = {};
    ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ImageInfo.imageView   = ImageView;
    ImageInfo.sampler     = Sampler;
    
    VkWriteDescriptorSet DescriptorWrite = {};
    ZERO_STRUCT(&DescriptorWrite);

    DescriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DescriptorWrite.dstSet           = m_DescriptorSet;
    DescriptorWrite.dstBinding       = Binding;
    DescriptorWrite.dstArrayElement  = 0;
    DescriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    DescriptorWrite.descriptorCount  = 1;
    DescriptorWrite.pBufferInfo      = nullptr;
    DescriptorWrite.pImageInfo       = &ImageInfo;
    DescriptorWrite.pTexelBufferView = nullptr;
    
    vkUpdateDescriptorSets(GetDevice()->GetDevice(), 1, &DescriptorWrite, 0, nullptr);
}

void FDescriptorSet::BindUniformBuffer(VkBuffer Buffer, uint32_t Binding)
{
    assert(m_DescriptorSet != VK_NULL_HANDLE);
    assert(Buffer != VK_NULL_HANDLE);
    
    VkDescriptorBufferInfo BufferInfo = {};
    BufferInfo.buffer = Buffer;
    BufferInfo.offset = 0;
    BufferInfo.range  = VK_WHOLE_SIZE;
    
    VkWriteDescriptorSet DescriptorWrite = {};
    ZERO_STRUCT(&DescriptorWrite);

    DescriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DescriptorWrite.dstSet           = m_DescriptorSet;
    DescriptorWrite.dstBinding       = Binding;
    DescriptorWrite.dstArrayElement  = 0;
    DescriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    DescriptorWrite.descriptorCount  = 1;
    DescriptorWrite.pBufferInfo      = &BufferInfo;
    DescriptorWrite.pImageInfo       = nullptr;
    DescriptorWrite.pTexelBufferView = nullptr;
    
    vkUpdateDescriptorSets(GetDevice()->GetDevice(), 1, &DescriptorWrite, 0, nullptr);
}

void FDescriptorSet::BindStorageBuffer(VkBuffer Buffer, uint32_t Binding)
{
    assert(m_DescriptorSet != VK_NULL_HANDLE);
    assert(Buffer != VK_NULL_HANDLE);

    VkDescriptorBufferInfo BufferInfo = {};
    BufferInfo.buffer = Buffer;
    BufferInfo.offset = 0;
    BufferInfo.range  = VK_WHOLE_SIZE;

    VkWriteDescriptorSet DescriptorWrite = {};
    ZERO_STRUCT(&DescriptorWrite);

    DescriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DescriptorWrite.dstSet           = m_DescriptorSet;
    DescriptorWrite.dstBinding       = Binding;
    DescriptorWrite.dstArrayElement  = 0;
    DescriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    DescriptorWrite.descriptorCount  = 1;
    DescriptorWrite.pBufferInfo      = &BufferInfo;
    DescriptorWrite.pImageInfo       = nullptr;
    DescriptorWrite.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(GetDevice()->GetDevice(), 1, &DescriptorWrite, 0, nullptr);
}

void FDescriptorSet::SetDebugName(const char* DebugName)
{
    if (FExtensions::vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT DebugNameInfo;
        ZERO_STRUCT(&DebugNameInfo);

        DebugNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        DebugNameInfo.objectType   = VK_OBJECT_TYPE_DESCRIPTOR_SET;
        DebugNameInfo.pObjectName  = DebugName;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_DescriptorSet);

        VkResult Result = FExtensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            std::cout << "Failed to set name '" << DebugNameInfo.pObjectName << "'.Error: " << Result << std::endl;
        }
    }
}
