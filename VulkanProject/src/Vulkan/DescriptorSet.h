#pragma once
#include "Core.h"

class DescriptorSet
{
public:
    // Allocates from pDescriptorPool and uses the layout from pPipeline
    static DescriptorSet* Create(class VulkanContext* pContext, class DescriptorPool* pDescriptorPool, class BasePipeline* pPipeline);

    DescriptorSet(VkDevice device, class DescriptorPool* pool);
    ~DescriptorSet();
    
    void BindStorageImage(VkImageView imageView, uint32_t binding);
    void BindUniformBuffer(VkBuffer buffer, uint32_t binding);
    
    VkDescriptorSet GetDescriptorSet() const
    {
        return m_DescriptorSet;
    }
    
private:
    class DescriptorPool* m_Pool;
    VkDevice        m_Device;
    VkDescriptorSet m_DescriptorSet;
};
