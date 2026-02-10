#pragma once
#include "DeviceChild.h"

class CDescriptorPool;

class CDescriptorSet : public CDeviceChild
{
public:
    // Allocates from pDescriptorPool and uses the layout from pPipeline
    static CDescriptorSet* Create(CDevice* pDevice, CDescriptorPool* pDescriptorPool, class CDescriptorSetLayout* pDescriptorSetLayout);

    CDescriptorSet(CDevice* pDevice, CDescriptorPool* pDescriptorPool);
    ~CDescriptorSet();
    
    void BindStorageImage(VkImageView ImageView, uint32_t Binding);
    void BindCombinedImageSampler(VkImageView ImageView, VkSampler Sampler, uint32_t Binding);
    void BindUniformBuffer(VkBuffer Buffer, uint32_t Binding);
    void BindStorageBuffer(VkBuffer Buffer, uint32_t Binding);
    void BindAccelerationStructure(VkAccelerationStructureKHR AccelerationStructure, uint32_t Binding);

    void SetDebugName(const char* DebugName);

    VkDescriptorSet GetDescriptorSet() const
    {
        return m_DescriptorSet;
    }
    
private:
    CDescriptorPool* m_pDescriptorPool;
    VkDescriptorSet  m_DescriptorSet;
};
