#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>

class FDevice;

struct FSamplerParams
{
    VkFilter             magFilter;
    VkFilter             minFilter;
    VkSamplerMipmapMode  mipmapMode;
    VkSamplerAddressMode addressModeU;
    VkSamplerAddressMode addressModeV;
    VkSamplerAddressMode addressModeW;
    float                mipLodBias;
    VkBool32             anisotropyEnable;
    float                maxAnisotropy;
    VkBool32             compareEnable;
    VkCompareOp          compareOp;
    float                minLod;
    float                maxLod;
    VkBorderColor        borderColor;
};

class FSampler
{
public:
    static FSampler* Create(FDevice* pDevice, const FSamplerParams& params);
    
    FSampler(VkDevice device);
    ~FSampler();

    VkSampler GetSampler() const
    {
        return m_Sampler;
    }

private:
    VkDevice  m_Device;
    VkSampler m_Sampler;
};
