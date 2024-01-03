#include "Framebuffer.h"
#include "RenderPass.h"
#include "VulkanContext.h"

Framebuffer* Framebuffer::Create(VulkanContext* pContext, const FramebufferParams& params)
{
    Framebuffer* pFrameBuffer = new Framebuffer(pContext->GetDevice());
    
    assert(params.pRenderPass != nullptr);

    VkFramebufferCreateInfo framebufferInfo;
    ZERO_STRUCT(&framebufferInfo);
    
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = params.pRenderPass->GetRenderPass();
    framebufferInfo.attachmentCount = params.AttachMentCount;
    framebufferInfo.pAttachments    = params.pAttachMents;
    framebufferInfo.width           = params.Width;
    framebufferInfo.height          = params.Height;
    framebufferInfo.layers          = 1;

    VkResult result = vkCreateFramebuffer(pFrameBuffer->m_Device, &framebufferInfo, nullptr, &pFrameBuffer->m_Framebuffer);
    if (result != VK_SUCCESS) 
    {
        std::cout << "vkCreateFramebuffer failed" << std::endl;
        return nullptr;
    }
    else
    {
        std::cout << "Created framebuffer" << std::endl;

        pFrameBuffer->m_Width  = params.Width;
        pFrameBuffer->m_Height = params.Height;
    }
    
    return pFrameBuffer;
}

Framebuffer::Framebuffer(VkDevice device)
    : m_Device(device)
    , m_Framebuffer(VK_NULL_HANDLE)
{
}

Framebuffer::~Framebuffer()
{
    if (m_Framebuffer != VK_NULL_HANDLE)
    {
        vkDestroyFramebuffer(m_Device, m_Framebuffer, nullptr);
        m_Framebuffer = VK_NULL_HANDLE;
    }

    m_Device = VK_NULL_HANDLE;
}
