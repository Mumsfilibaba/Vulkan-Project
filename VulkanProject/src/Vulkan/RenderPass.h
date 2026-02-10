#pragma once
#include "DeviceChild.h"
#include <vulkan/vulkan.h>

struct SRenderPassAttachment
{
    VkFormat            Format = VK_FORMAT_UNDEFINED;
    VkAttachmentLoadOp  LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkImageLayout       InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout       FinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
};

struct SRenderPassParams
{
    SRenderPassAttachment* pColorAttachments = nullptr;
    uint32_t ColorAttachmentCount = 0;
    SRenderPassAttachment* pDepthAttachment = nullptr;
};

class CRenderPass : public CDeviceChild
{
public:
    static CRenderPass* Create(class CDevice* pDevice, const SRenderPassParams& Params);
    
    CRenderPass(CDevice* pDevice);
    ~CRenderPass();

    void SetDebugName(const char* DebugName);

    VkRenderPass GetRenderPass() const
    {
        return m_RenderPass;
    }
    
private:
    VkRenderPass m_RenderPass;
};
