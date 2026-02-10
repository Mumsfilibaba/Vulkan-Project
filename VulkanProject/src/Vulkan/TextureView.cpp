#include "TextureView.h"
#include "Device.h"
#include "Texture.h"
#include "Extensions.h"

CTextureView* CTextureView::Create(CDevice* pDevice, const STextureViewParams& Params)
{
    CTextureView* pTextureView = new CTextureView(pDevice);

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

CTextureView::CTextureView(CDevice* pDevice)
    : CDeviceChild(pDevice)
    , m_ImageView(VK_NULL_HANDLE)
{
}

CTextureView::~CTextureView()
{
    if (m_ImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(GetDevice()->GetDevice(), m_ImageView, nullptr);
        m_ImageView = VK_NULL_HANDLE;
    }
}

void CTextureView::SetDebugName(const char* DebugName)
{
    if (Extensions::vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT DebugNameInfo;
        ZERO_STRUCT(&DebugNameInfo);
        
        DebugNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        DebugNameInfo.objectType   = VK_OBJECT_TYPE_IMAGE_VIEW;
        DebugNameInfo.pObjectName  = DebugName;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_ImageView);

        VkResult Result = Extensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            LOG("Failed to set name '%s'. Error: %d\n", DebugNameInfo.pObjectName, Result);
        }
    }
}
