#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>

struct RenderPassAttachment
{
    VkFormat            Format        = VK_FORMAT_UNDEFINED;
    VkAttachmentLoadOp  LoadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp StoreOp       = VK_ATTACHMENT_STORE_OP_STORE;
    VkImageLayout       initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout       finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
};

struct RenderPassParams
{
    RenderPassAttachment* pColorAttachments    = nullptr;
    uint32_t              ColorAttachmentCount = 0;
};

class RenderPass
{
public:
    static RenderPass* Create(class Device* pDevice, const RenderPassParams& params);
    
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
