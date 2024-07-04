#pragma once
#include "DeviceChild.h"

class FDescriptorPool;

class FDescriptorSet : public FDeviceChild
{
public:
    // Allocates from pDescriptorPool and uses the layout from pPipeline
    static FDescriptorSet* Create(FDevice* pDevice, FDescriptorPool* pDescriptorPool, class FDescriptorSetLayout* pDescriptorSetLayout);

    FDescriptorSet(FDevice* pDevice, FDescriptorPool* pDescriptorPool);
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
    FDescriptorPool* m_pDescriptorPool;
    VkDescriptorSet  m_DescriptorSet;
};
