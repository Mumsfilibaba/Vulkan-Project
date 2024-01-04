#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>

class Device;

struct SamplerParams
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

class Sampler
{
public:
    static Sampler* Create(Device* pDevice, const SamplerParams& params);
    
    Sampler(VkDevice device);
    ~Sampler();

    VkSampler GetSampler() const
    {
        return m_Sampler;
    }

private:
    VkDevice  m_Device;
    VkSampler m_Sampler;
};
