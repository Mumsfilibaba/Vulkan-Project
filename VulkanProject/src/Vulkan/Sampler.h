#pragma once
#include "DeviceChild.h"
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

class FSampler : public FDeviceChild
{
public:
    static FSampler* Create(FDevice* pDevice, const FSamplerParams& params);
    
    FSampler(FDevice* pDevice);
    ~FSampler();

    VkSampler GetSampler() const
    {
        return m_Sampler;
    }

private:
    VkSampler m_Sampler;
};
