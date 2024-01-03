#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>

class VulkanContext;

struct TextureParams
{
    VkImageUsageFlags Usage     = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    VkFormat          Format    = VK_FORMAT_UNDEFINED;
    VkImageType       ImageType = VK_IMAGE_TYPE_2D;
    
    uint32_t Width  = 0;
    uint32_t Height = 0;
};

class Texture
{
public:
    static Texture* Create(VulkanContext* pContext, const TextureParams& params);
    static Texture* CreateWithData(VulkanContext* pContext, const TextureParams& params, const void* pSource);

    Texture(VulkanContext* pContext);
    ~Texture();

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
    VulkanContext* m_pContext;
    VkImage        m_Image;
    VkDeviceMemory m_Memory;
    VkFormat       m_Format;
    uint32_t       m_Width;
    uint32_t       m_Height;
};
