#pragma once
#include "DeviceChild.h"
#include <vulkan/vulkan.h>

class CRenderPass;

struct SFramebufferParams
{
    CRenderPass* pRenderPass     = nullptr;
    VkImageView* pAttachMents    = nullptr;
    uint32_t     AttachmentCount = 0;
    uint32_t     Width           = 0;
    uint32_t     Height          = 0;
};

class CFramebuffer : public CDeviceChild
{
public:
    static CFramebuffer* Create(CDevice* pDevice, const SFramebufferParams& Params);
    
    CFramebuffer(CDevice* pDevice);
    ~CFramebuffer();

    void SetDebugName(const char* DebugName);

    VkExtent2D GetExtent() const
    {
        return { m_Width, m_Height };
    }
    
    VkFramebuffer GetFramebuffer() const
    {
        return m_Framebuffer;
    }

private:
    VkFramebuffer m_Framebuffer;
    uint32_t      m_Width;
    uint32_t      m_Height;
};

