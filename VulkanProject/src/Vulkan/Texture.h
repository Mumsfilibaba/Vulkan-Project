#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>

class FDevice;

struct FTextureParams
{
    VkImageUsageFlags Usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    VkFormat          Format        = VK_FORMAT_UNDEFINED;
    VkImageType       ImageType     = VK_IMAGE_TYPE_2D;
    VkImageLayout     InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    
    uint32_t Width  = 0;
    uint32_t Height = 0;
};

class FTexture
{
public:
    static FTexture* Create(FDevice* pDevice, const FTextureParams& params);
    static FTexture* CreateWithData(FDevice* pDevice, const FTextureParams& params, const void* pSource);

    FTexture(FDevice* pDevice);
    ~FTexture();

    VkImage GetImage() const
    {
        return m_Image;
    }

    VkFormat GetFormat() const
    {
        return m_Format;
    }

    uint32_t GetWidth() const
    {
        return m_Width;
    }

    uint32_t GetHeight() const
    {
        return m_Height;
    }

private:
    FDevice*       m_pDevice;
    VkImage        m_Image;
    VkDeviceMemory m_Memory;
    VkFormat       m_Format;
    uint32_t       m_Width;
    uint32_t       m_Height;
};
