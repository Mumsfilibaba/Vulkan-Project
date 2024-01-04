#include "TextureView.h"
#include "Device.h"
#include "Texture.h"

TextureView* TextureView::Create(Device* pDevice, const TextureViewParams& params)
{
    TextureView* pTextureView = new TextureView(pDevice->GetDevice());

    VkImageViewCreateInfo textureViewCreateInfo = {};
    ZERO_STRUCT(&textureViewCreateInfo);
    
    textureViewCreateInfo.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    textureViewCreateInfo.image                       = params.pTexture->GetImage();
    textureViewCreateInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
    textureViewCreateInfo.format                      = params.pTexture->GetFormat();
    textureViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    textureViewCreateInfo.subresourceRange.levelCount = 1;
    textureViewCreateInfo.subresourceRange.layerCount = 1;

    VkResult result = vkCreateImageView(pDevice->GetDevice(), &textureViewCreateInfo, nullptr, &pTextureView->m_ImageView);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkCreateImageView failed\n";
        return nullptr;
    }
    else
    {
        std::cout << "Created ImageView\n";
        return pTextureView;
    }
}

TextureView::TextureView(VkDevice device)
    : m_Device(device)
    , m_ImageView(VK_NULL_HANDLE)
{
}

TextureView::~TextureView()
{
    if (m_ImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(m_Device, m_ImageView, nullptr);
        m_ImageView = VK_NULL_HANDLE;
    }

    m_Device = VK_NULL_HANDLE;
}
