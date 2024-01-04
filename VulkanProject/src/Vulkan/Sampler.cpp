#include "Sampler.h"
#include "Device.h"

Sampler* Sampler::Create(Device* pDevice, const SamplerParams& params)
{
    Sampler* pSampler = new Sampler(pDevice->GetDevice());
    
    VkSamplerCreateInfo samplerCreateInfo;
    ZERO_STRUCT(&samplerCreateInfo);

    samplerCreateInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter               = params.magFilter;
    samplerCreateInfo.minFilter               = params.minFilter;
    samplerCreateInfo.mipmapMode              = params.mipmapMode;
    samplerCreateInfo.addressModeU            = params.addressModeU;
    samplerCreateInfo.addressModeV            = params.addressModeV;
    samplerCreateInfo.addressModeW            = params.addressModeW;
    samplerCreateInfo.mipLodBias              = params.mipLodBias;
    samplerCreateInfo.anisotropyEnable        = params.anisotropyEnable;
    samplerCreateInfo.maxAnisotropy           = params.maxAnisotropy;
    samplerCreateInfo.compareEnable           = params.compareEnable;
    samplerCreateInfo.compareOp               = params.compareOp;
    samplerCreateInfo.minLod                  = params.minLod;
    samplerCreateInfo.maxLod                  = params.maxLod;
    samplerCreateInfo.borderColor             = params.borderColor;
    samplerCreateInfo.unnormalizedCoordinates = false;

    VkResult result = vkCreateSampler(pDevice->GetDevice(), &samplerCreateInfo, nullptr, &pSampler->m_Sampler);
    if (result != VK_SUCCESS)
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
    
Sampler::Sampler(VkDevice device)
    : m_Device(device)
    , m_Sampler(VK_NULL_HANDLE)
{
}

Sampler::~Sampler()
{
    if (m_Sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(m_Device, m_Sampler, nullptr);
        m_Sampler = VK_NULL_HANDLE;
    }
}
