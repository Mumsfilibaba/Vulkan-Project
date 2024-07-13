#include "Sampler.h"
#include "Device.h"
#include "Extensions.h"

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

void FSampler::SetDebugName(const char* DebugName)
{
    if (FExtensions::vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT DebugNameInfo;
        ZERO_STRUCT(&DebugNameInfo);
        
        DebugNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        DebugNameInfo.objectType   = VK_OBJECT_TYPE_SAMPLER;
        DebugNameInfo.pObjectName  = DebugName;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_Sampler);

        VkResult Result = FExtensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            std::cout << "Failed to set name '" << DebugNameInfo.pObjectName << "'.Error: " << Result << std::endl;
        }
    }
}
