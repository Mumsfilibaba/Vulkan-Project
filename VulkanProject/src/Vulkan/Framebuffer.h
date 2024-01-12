#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>

class FRenderPass;

struct FFramebufferParams
{
    FRenderPass* pRenderPass     = nullptr;
    VkImageView* pAttachMents    = nullptr;
    uint32_t     AttachMentCount = 0;
    uint32_t     Width           = 0;
    uint32_t     Height          = 0;
};

class FFramebuffer
{
public:
    static FFramebuffer* Create(class FDevice* pDevice, const FFramebufferParams& params);
    
    FFramebuffer(VkDevice device);
    ~FFramebuffer();

    VkExtent2D GetExtent() const
    {
        return { m_Width, m_Height };
    }
    
    VkFramebuffer GetFramebuffer() const
    {
        return m_Framebuffer;
    }

    
private:
    VkDevice      m_Device;
    VkFramebuffer m_Framebuffer;
    uint32_t      m_Width;
    uint32_t      m_Height;
};

