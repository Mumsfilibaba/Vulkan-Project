#pragma once
#include "Core.h"

class FDescriptorSet
{
public:
    // Allocates from pDescriptorPool and uses the layout from pPipeline
    static FDescriptorSet* Create(class FDevice* pDevice, class FDescriptorPool* pDescriptorPool, class FDescriptorSetLayout* pDescriptorSetLayout);

    FDescriptorSet(VkDevice device, class FDescriptorPool* pool);
    ~FDescriptorSet();
    
    void BindStorageImage(VkImageView imageView, uint32_t binding);
    void BindCombinedImageSampler(VkImageView imageView, VkSampler sampler, uint32_t binding);
    void BindUniformBuffer(VkBuffer buffer, uint32_t binding);
    void BindStorageBuffer(VkBuffer buffer, uint32_t binding);
    
    VkDescriptorSet GetDescriptorSet() const
    {
        return m_DescriptorSet;
    }
    
private:
    class FDescriptorPool* m_Pool;
    VkDevice               m_Device;
    VkDescriptorSet        m_DescriptorSet;
};
