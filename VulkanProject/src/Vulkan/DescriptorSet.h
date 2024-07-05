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
    
    void BindStorageImage(VkImageView ImageView, uint32_t Binding);
    void BindCombinedImageSampler(VkImageView ImageView, VkSampler Sampler, uint32_t Binding);
    void BindUniformBuffer(VkBuffer Buffer, uint32_t Binding);
    void BindStorageBuffer(VkBuffer Buffer, uint32_t Binding);
    
    VkDescriptorSet GetDescriptorSet() const
    {
        return m_DescriptorSet;
    }
    
private:
    FDescriptorPool* m_pDescriptorPool;
    VkDescriptorSet  m_DescriptorSet;
};
