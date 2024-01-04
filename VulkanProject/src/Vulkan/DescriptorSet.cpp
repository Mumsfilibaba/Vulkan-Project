#include "DescriptorSet.h"
#include "Device.h"
#include "DescriptorPool.h"
#include "PipelineState.h"
#include "DescriptorSetLayout.h"
#include <vulkan/vulkan.h>

// Allocates from pDescriptorPool and uses the layout from pPipeline
DescriptorSet* DescriptorSet::Create(Device* pDevice, DescriptorPool* pDescriptorPool, DescriptorSetLayout* pDescriptorSetLayout)
{
    assert(pDevice != nullptr);
    assert(pDescriptorPool != nullptr);
    assert(pDescriptorSetLayout != nullptr);
    
    DescriptorSet* pDescriptorSet = new DescriptorSet(pDevice->GetDevice(), pDescriptorPool);
    
    VkDescriptorSetAllocateInfo allocateInfo;
    ZERO_STRUCT(&allocateInfo);
    
    VkDescriptorSetLayout descriptorLayout = pDescriptorSetLayout->GetDescriptorSetLayout();
    allocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts        = &descriptorLayout;
    allocateInfo.descriptorPool     = pDescriptorPool->GetPool();
    
    VkResult result = vkAllocateDescriptorSets(pDescriptorSet->m_Device, &allocateInfo, &pDescriptorSet->m_DescriptorSet);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkAllocateDescriptorSets failed\n";
        return nullptr;
    }
    
    return pDescriptorSet;
}

DescriptorSet::DescriptorSet(VkDevice device, class DescriptorPool* pool)
    : m_Pool(pool)
    , m_Device(device)
    , m_DescriptorSet(VK_NULL_HANDLE)
{
    assert(m_Pool != nullptr);
}

DescriptorSet::~DescriptorSet()
{
    if (m_DescriptorSet)
    {
        vkFreeDescriptorSets(m_Device, m_Pool->GetPool(), 1, &m_DescriptorSet);
        m_DescriptorSet = VK_NULL_HANDLE;
    }
    
    m_Device = VK_NULL_HANDLE;
}

void DescriptorSet::BindStorageImage(VkImageView imageView, uint32_t binding)
{
    assert(m_DescriptorSet != VK_NULL_HANDLE);
    assert(imageView != VK_NULL_HANDLE);
    
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo.imageView   = imageView;
    imageInfo.sampler     = VK_NULL_HANDLE;
    
    VkWriteDescriptorSet descriptorWrite = {};
    ZERO_STRUCT(&descriptorWrite);

    descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet           = m_DescriptorSet;
    descriptorWrite.dstBinding       = binding;
    descriptorWrite.dstArrayElement  = 0;
    descriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorWrite.descriptorCount  = 1;
    descriptorWrite.pBufferInfo      = nullptr;
    descriptorWrite.pImageInfo       = &imageInfo;
    descriptorWrite.pTexelBufferView = nullptr;
    
    vkUpdateDescriptorSets(m_Device, 1, &descriptorWrite, 0, nullptr);
}

void DescriptorSet::BindCombinedImageSampler(VkImageView imageView, VkSampler sampler, uint32_t binding)
{
    assert(m_DescriptorSet != VK_NULL_HANDLE);
    assert(sampler != VK_NULL_HANDLE);
    assert(imageView != VK_NULL_HANDLE);
    
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView   = imageView;
    imageInfo.sampler     = sampler;
    
    VkWriteDescriptorSet descriptorWrite = {};
    ZERO_STRUCT(&descriptorWrite);

    descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet           = m_DescriptorSet;
    descriptorWrite.dstBinding       = binding;
    descriptorWrite.dstArrayElement  = 0;
    descriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount  = 1;
    descriptorWrite.pBufferInfo      = nullptr;
    descriptorWrite.pImageInfo       = &imageInfo;
    descriptorWrite.pTexelBufferView = nullptr;
    
    vkUpdateDescriptorSets(m_Device, 1, &descriptorWrite, 0, nullptr);
}

void DescriptorSet::BindUniformBuffer(VkBuffer buffer, uint32_t binding)
{
    assert(m_DescriptorSet != VK_NULL_HANDLE);
    assert(buffer != VK_NULL_HANDLE);
    
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range  = VK_WHOLE_SIZE;
    
    VkWriteDescriptorSet descriptorWrite = {};
    ZERO_STRUCT(&descriptorWrite);

    descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet           = m_DescriptorSet;
    descriptorWrite.dstBinding       = binding;
    descriptorWrite.dstArrayElement  = 0;
    descriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount  = 1;
    descriptorWrite.pBufferInfo      = &bufferInfo;
    descriptorWrite.pImageInfo       = nullptr;
    descriptorWrite.pTexelBufferView = nullptr;
    
    vkUpdateDescriptorSets(m_Device, 1, &descriptorWrite, 0, nullptr);
}

void DescriptorSet::BindStorageBuffer(VkBuffer buffer, uint32_t binding)
{
    assert(m_DescriptorSet != VK_NULL_HANDLE);
    assert(buffer != VK_NULL_HANDLE);

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range  = VK_WHOLE_SIZE;

    VkWriteDescriptorSet descriptorWrite = {};
    ZERO_STRUCT(&descriptorWrite);

    descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet           = m_DescriptorSet;
    descriptorWrite.dstBinding       = binding;
    descriptorWrite.dstArrayElement  = 0;
    descriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrite.descriptorCount  = 1;
    descriptorWrite.pBufferInfo      = &bufferInfo;
    descriptorWrite.pImageInfo       = nullptr;
    descriptorWrite.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_Device, 1, &descriptorWrite, 0, nullptr);
}
