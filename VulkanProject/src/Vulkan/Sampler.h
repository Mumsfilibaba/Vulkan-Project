#pragma once
#include "DeviceChild.h"
#include <vulkan/vulkan.h>

class CDevice;

struct SSamplerParams
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

class CSampler : public CDeviceChild
{
public:
    static CSampler* Create(CDevice* pDevice, const SSamplerParams& Params);
    
    CSampler(CDevice* pDevice);
    ~CSampler();

    void SetDebugName(const char* DebugName);
    
    VkSampler GetSampler() const
    {
        return m_Sampler;
    }

private:
    VkSampler m_Sampler;
};
