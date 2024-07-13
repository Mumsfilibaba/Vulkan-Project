#pragma once
#include "DeviceChild.h"

class FDevice;
class FTextureView;

class FBindlessManager : public FDeviceChild
{
public:
    static FBindlessManager* Create(FDevice* pDevice);

    FBindlessManager(FDevice* pDevice);
    ~FBindlessManager();

    // Returns a Bindless ID
    uint32_t AddImageView(VkImageView ImageView, VkSampler Sampler);

    VkDescriptorSet GetDescriptorSet()
    {
        return m_DescriptorSet;
    }
    
    VkDescriptorSetLayout GetDescriptorSetLayout()
    {
        return m_DescriptorSetLayout;
    }

private:
    VkDescriptorSet       m_DescriptorSet;
    VkDescriptorPool      m_DescriptorPool;
    VkDescriptorSetLayout m_DescriptorSetLayout;
    uint32_t              m_NextTextureBinding;
};
