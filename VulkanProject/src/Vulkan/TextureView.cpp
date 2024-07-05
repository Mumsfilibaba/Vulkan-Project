#include "TextureView.h"
#include "Device.h"
#include "Texture.h"

FTextureView* FTextureView::Create(FDevice* pDevice, const FTextureViewParams& Params)
{
    FTextureView* pTextureView = new FTextureView(pDevice);

    VkImageViewCreateInfo TextureViewCreateInfo = {};
    ZERO_STRUCT(&TextureViewCreateInfo);
    
    TextureViewCreateInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    TextureViewCreateInfo.image                           = Params.pTexture->GetImage();
    TextureViewCreateInfo.viewType                        = pTextureView->m_ViewType = Params.ViewType;
    TextureViewCreateInfo.format                          = Params.pTexture->GetFormat();
    TextureViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    TextureViewCreateInfo.subresourceRange.levelCount     = 1;
    TextureViewCreateInfo.subresourceRange.baseArrayLayer = Params.BaseArraySlice;
    TextureViewCreateInfo.subresourceRange.layerCount     = Params.NumArraySlices;

    VkResult result = vkCreateImageView(pDevice->GetDevice(), &TextureViewCreateInfo, nullptr, &pTextureView->m_ImageView);
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

FTextureView::FTextureView(FDevice* pDevice)
    : FDeviceChild(pDevice)
    , m_ImageView(VK_NULL_HANDLE)
{
}

FTextureView::~FTextureView()
{
    if (m_ImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(GetDevice()->GetDevice(), m_ImageView, nullptr);
        m_ImageView = VK_NULL_HANDLE;
    }
}
