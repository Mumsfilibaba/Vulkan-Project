#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>

class FDevice;
class FTexture;

struct FTextureViewParams
{
    FTexture*       pTexture       = nullptr;
    VkImageViewType ViewType       = VK_IMAGE_VIEW_TYPE_2D;
    uint32_t        BaseArraySlice = 0;
    uint32_t        NumArraySlices = 1;
};

class FTextureView
{
public:
    static FTextureView* Create(FDevice* pDevice, const FTextureViewParams& params);

    FTextureView(VkDevice device);
    ~FTextureView();

    VkImageView GetImageView() const
    {
        return m_ImageView;
    }

    VkImageViewType GetViewType() const
    {
        return m_ViewType;
    }
    
private:
    VkDevice        m_Device;
    VkImageView     m_ImageView;
    VkImageViewType m_ViewType;
};
