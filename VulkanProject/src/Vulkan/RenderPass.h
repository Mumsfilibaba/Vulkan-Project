#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>

struct RenderPassAttachment
{
    VkFormat Format = VK_FORMAT_UNDEFINED;
};

struct RenderPassParams
{
    RenderPassAttachment* pColorAttachments    = nullptr;
    uint32_t              ColorAttachmentCount = 0;
};

class RenderPass
{
public:
    static RenderPass* Create(class VulkanContext* pContext, const RenderPassParams& params);
    
    RenderPass(VkDevice device);
    ~RenderPass();

    VkRenderPass GetRenderPass() const
    {
        return m_RenderPass;
    }
    
private:
    VkDevice     m_Device;
    VkRenderPass m_RenderPass;
};
