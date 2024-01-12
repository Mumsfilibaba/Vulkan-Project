#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>

class FDevice;
class FTexture;

struct FTextureViewParams
{
    FTexture* pTexture = nullptr;
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

private:
    VkDevice    m_Device;
    VkImageView m_ImageView;
};