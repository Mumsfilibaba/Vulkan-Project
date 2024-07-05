#pragma once
#include "DeviceChild.h"
#include <vulkan/vulkan.h>

class FDevice;

struct FSamplerParams
{
    VkFilter             MagFilter;
    VkFilter             MinFilter;
    VkSamplerMipmapMode  MipmapMode;
    VkSamplerAddressMode AddressModeU;
    VkSamplerAddressMode AddressModeV;
    VkSamplerAddressMode AddressModeW;
    float                MipLodBias;
    VkBool32             AnisotropyEnable;
    float                MaxAnisotropy;
    VkBool32             CompareEnable;
    VkCompareOp          CompareOp;
    float                MinLod;
    float                MaxLod;
    VkBorderColor        BorderColor;
};

class FSampler : public FDeviceChild
{
public:
    static FSampler* Create(FDevice* pDevice, const FSamplerParams& Params);
    
    FSampler(FDevice* pDevice);
    ~FSampler();

    VkSampler GetSampler() const
    {
        return m_Sampler;
    }

private:
    VkSampler m_Sampler;
};
