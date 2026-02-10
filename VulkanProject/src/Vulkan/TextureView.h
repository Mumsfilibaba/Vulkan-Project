#pragma once
#include "DeviceChild.h"
#include <vulkan/vulkan.h>

class CDevice;
class CTexture;

struct STextureViewParams
{
    CTexture*       pTexture       = nullptr;
    VkImageViewType ViewType       = VK_IMAGE_VIEW_TYPE_2D;
    uint32_t        BaseArraySlice = 0;
    uint32_t        NumArraySlices = 1;
};

class CTextureView : public CDeviceChild
{
public:
    static CTextureView* Create(CDevice* pDevice, const STextureViewParams& Params);

    CTextureView(CDevice* pDevice);
    ~CTextureView();

    void SetDebugName(const char* DebugName);

    VkImageView GetImageView() const
    {
        return m_ImageView;
    }

    VkImageViewType GetViewType() const
    {
        return m_ViewType;
    }
    
private:
    VkImageView     m_ImageView;
    VkImageViewType m_ViewType;
};
