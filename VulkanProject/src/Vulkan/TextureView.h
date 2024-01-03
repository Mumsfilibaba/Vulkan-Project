#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>

class VulkanContext;
class Texture;

struct TextureViewParams
{
    Texture* pTexture = nullptr;
};

class TextureView
{
public:
    static TextureView* Create(VulkanContext* pContext, const TextureViewParams& params);

    TextureView(VkDevice device);
    ~TextureView();

    VkImageView GetImageView() const
    {
        return m_ImageView;
    }

private:
    VkDevice    m_Device;
    VkImageView m_ImageView;
};