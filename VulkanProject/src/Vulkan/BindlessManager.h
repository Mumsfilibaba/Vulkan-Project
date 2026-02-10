#pragma once
#include "DeviceChild.h"

class CDevice;
class CTextureView;

class CBindlessManager : public CDeviceChild
{
public:
    static CBindlessManager* Create(CDevice* pDevice);

    static constexpr const uint32_t InvalidBindlessID = static_cast<uint32_t>(-1);
    
    CBindlessManager(CDevice* pDevice);
    ~CBindlessManager();

    // Returns a Bindless ID
    uint32_t AddImageView(VkImageView ImageView, VkSampler Sampler);
    
    // Removes a ImageView and frees the Bindless ID
    void RemoveImageView(VkImageView ImageView);

    VkDescriptorSet GetDescriptorSet()
    {
        return m_DescriptorSet;
    }
    
    VkDescriptorSetLayout GetDescriptorSetLayout()
    {
        return m_DescriptorSetLayout;
    }

private:
    using BindlessMap = std::unordered_map<VkImageView, uint32_t>;
    
    VkDescriptorSet       m_DescriptorSet;
    VkDescriptorPool      m_DescriptorPool;
    VkDescriptorSetLayout m_DescriptorSetLayout;
    uint32_t              m_NextTextureBinding;
    uint32_t              m_MaxTextureBinding;
    std::vector<uint32_t> m_TextureFreeList;
    BindlessMap           m_TextureBindings;
};
