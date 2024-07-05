#pragma once
#include "DeviceChild.h"
#include <vulkan/vulkan.h>

struct FRenderPassAttachment
{
    VkFormat            Format = VK_FORMAT_UNDEFINED;
    VkAttachmentLoadOp  LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkImageLayout       InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout       FinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
};

struct FRenderPassParams
{
    FRenderPassAttachment* pColorAttachments = nullptr;
    uint32_t ColorAttachmentCount = 0;
};

class FRenderPass : public FDeviceChild
{
public:
    static FRenderPass* Create(class FDevice* pDevice, const FRenderPassParams& Params);
    
    FRenderPass(FDevice* pDevice);
    ~FRenderPass();

    VkRenderPass GetRenderPass() const
    {
        return m_RenderPass;
    }
    
private:
    VkRenderPass m_RenderPass;
};
