#include "DescriptorSet.h"
#include "VulkanContext.h"
#include "DescriptorPool.h"
#include "PipelineState.h"
#include <vulkan/vulkan.h>

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
    descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.pNext            = nullptr;
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

void DescriptorSet::BindUniformBuffer(VkBuffer buffer, uint32_t binding)
{
    assert(m_DescriptorSet != VK_NULL_HANDLE);
    assert(buffer != VK_NULL_HANDLE);
    
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range  = VK_WHOLE_SIZE;
    
    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.pNext            = nullptr;
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

// Allocates from pDescriptorPool and uses the layout from pPipeline
DescriptorSet* DescriptorSet::Create(VulkanContext* pContext, DescriptorPool* pDescriptorPool, BasePipeline* pPipeline)
{
    assert(pContext != nullptr);
    assert(pDescriptorPool != nullptr);
    assert(pPipeline != nullptr);
    
    DescriptorSet* newSet = new DescriptorSet(pContext->GetDevice(), pDescriptorPool);
    
    VkDescriptorSetAllocateInfo allocateInfo = {};
    allocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pNext              = nullptr;
    
    VkDescriptorSetLayout descriptorLayout = pPipeline->GetDescriptorSetLayout();
    allocateInfo.pSetLayouts    = &descriptorLayout;
    allocateInfo.descriptorPool = pDescriptorPool->GetPool();
    
    VkResult result = vkAllocateDescriptorSets(newSet->m_Device, &allocateInfo, &newSet->m_DescriptorSet);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkAllocateDescriptorSets failed" << std::endl;
        return nullptr;
    }
    
    return newSet;
}
