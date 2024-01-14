#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>

struct FRenderPassAttachment
{
    VkFormat            Format        = VK_FORMAT_UNDEFINED;
    VkAttachmentLoadOp  LoadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp StoreOp       = VK_ATTACHMENT_STORE_OP_STORE;
    VkImageLayout       initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout       finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
};

struct FRenderPassParams
{
    FRenderPassAttachment* pColorAttachments    = nullptr;
    uint32_t               ColorAttachmentCount = 0;
};

class FRenderPass
{
public:
    static FRenderPass* Create(class FDevice* pDevice, const FRenderPassParams& params);
    
    FRenderPass(VkDevice device);
    ~FRenderPass();

    VkRenderPass GetRenderPass() const
    {
        return m_RenderPass;
    }
    
private:
    VkDevice     m_Device;
    VkRenderPass m_RenderPass;
};
