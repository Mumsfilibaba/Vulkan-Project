#include "Sampler.h"
#include "Device.h"

FSampler* FSampler::Create(FDevice* pDevice, const FSamplerParams& Params)
{
    FSampler* pSampler = new FSampler(pDevice);
    
    VkSamplerCreateInfo SamplerCreateInfo;
    ZERO_STRUCT(&SamplerCreateInfo);

    SamplerCreateInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerCreateInfo.magFilter               = Params.MagFilter;
    SamplerCreateInfo.minFilter               = Params.MinFilter;
    SamplerCreateInfo.mipmapMode              = Params.MipmapMode;
    SamplerCreateInfo.addressModeU            = Params.AddressModeU;
    SamplerCreateInfo.addressModeV            = Params.AddressModeV;
    SamplerCreateInfo.addressModeW            = Params.AddressModeW;
    SamplerCreateInfo.mipLodBias              = Params.MipLodBias;
    SamplerCreateInfo.anisotropyEnable        = Params.AnisotropyEnable;
    SamplerCreateInfo.maxAnisotropy           = Params.MaxAnisotropy;
    SamplerCreateInfo.compareEnable           = Params.CompareEnable;
    SamplerCreateInfo.compareOp               = Params.CompareOp;
    SamplerCreateInfo.minLod                  = Params.MinLod;
    SamplerCreateInfo.maxLod                  = Params.MaxLod;
    SamplerCreateInfo.borderColor             = Params.BorderColor;
    SamplerCreateInfo.unnormalizedCoordinates = false;

    VkResult Result = vkCreateSampler(pDevice->GetDevice(), &SamplerCreateInfo, nullptr, &pSampler->m_Sampler);
    if (Result != VK_SUCCESS)
    {
        std::cout << "vkCreateSampler failed\n";
        return nullptr;
    }
    else
    {
        std::cout << "Created sampler\n";
        return pSampler;
    }
}
    
FSampler::FSampler(FDevice* pDevice)
    : FDeviceChild(pDevice)
    , m_Sampler(VK_NULL_HANDLE)
{
}

FSampler::~FSampler()
{
    if (m_Sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(GetDevice()->GetDevice(), m_Sampler, nullptr);
        m_Sampler = VK_NULL_HANDLE;
    }
}
