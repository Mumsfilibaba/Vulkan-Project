#pragma once
#include "Core.h"

#include <vulkan/vulkan.h>

class RenderPass;

struct FramebufferParams
{
    RenderPass* pRenderPass = nullptr;
    VkImageView* pAttachMents = nullptr;
    uint32_t AttachMentCount = 0;
    uint32_t Width    = 0;
    uint32_t Height    = 0;
};

class Framebuffer
{
public:
    Framebuffer(VkDevice device);
    ~Framebuffer();

    inline VkExtent2D GetExtent() const
    {
        return { m_Width, m_Height };
    }
    
    inline VkFramebuffer GetFramebuffer() const
    {
        return m_Framebuffer;
    }

    static Framebuffer* Create(class VulkanContext* pContext, const FramebufferParams& params);
    
private:
    VkDevice m_Device;
    VkFramebuffer m_Framebuffer;
    uint32_t m_Width;
    uint32_t m_Height;
};

