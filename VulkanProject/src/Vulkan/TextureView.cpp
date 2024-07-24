#include "TextureView.h"
#include "Device.h"
#include "Texture.h"
#include "Extensions.h"

FTextureView* FTextureView::Create(FDevice* pDevice, const FTextureViewParams& Params)
{
    FTextureView* pTextureView = new FTextureView(pDevice);

    VkImageViewCreateInfo TextureViewCreateInfo = {};
    ZERO_STRUCT(&TextureViewCreateInfo);
    
    TextureViewCreateInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    TextureViewCreateInfo.image                           = Params.pTexture->GetImage();
    TextureViewCreateInfo.viewType                        = pTextureView->m_ViewType = Params.ViewType;
    TextureViewCreateInfo.format                          = Params.pTexture->GetFormat();
    TextureViewCreateInfo.subresourceRange.levelCount     = 1;
    TextureViewCreateInfo.subresourceRange.baseArrayLayer = Params.BaseArraySlice;
    TextureViewCreateInfo.subresourceRange.layerCount     = Params.NumArraySlices;

    if (TextureViewCreateInfo.format == VK_FORMAT_D24_UNORM_S8_UINT)
    {
        TextureViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else
    {
        TextureViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    
    VkResult result = vkCreateImageView(pDevice->GetDevice(), &TextureViewCreateInfo, nullptr, &pTextureView->m_ImageView);
    if (result != VK_SUCCESS)
    {
        LOG("vkCreateImageView failed\n");
        return nullptr;
    }
    else
    {
        LOG("Created ImageView\n");
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

void FTextureView::SetDebugName(const char* DebugName)
{
    if (FExtensions::vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT DebugNameInfo;
        ZERO_STRUCT(&DebugNameInfo);
        
        DebugNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        DebugNameInfo.objectType   = VK_OBJECT_TYPE_IMAGE_VIEW;
        DebugNameInfo.pObjectName  = DebugName;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_ImageView);

        VkResult Result = FExtensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            LOG("Failed to set name '%s'. Error: %d\n", DebugNameInfo.pObjectName, Result);
        }
    }
}
