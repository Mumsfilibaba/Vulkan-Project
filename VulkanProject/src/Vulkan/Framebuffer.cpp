#include "Framebuffer.h"
#include "RenderPass.h"
#include "Device.h"

FFramebuffer* FFramebuffer::Create(FDevice* pDevice, const FFramebufferParams& params)
{
    FFramebuffer* pFramebuffer = new FFramebuffer(pDevice->GetDevice());
    
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

    VkResult result = vkCreateFramebuffer(pFramebuffer->m_Device, &framebufferInfo, nullptr, &pFramebuffer->m_Framebuffer);
    if (result != VK_SUCCESS) 
    {
        std::cout << "vkCreateFramebuffer failed\n";
        return nullptr;
    }
    else
    {
        std::cout << "Created Framebuffer\n";

        pFramebuffer->m_Width  = params.Width;
        pFramebuffer->m_Height = params.Height;
    }
    
    return pFramebuffer;
}

FFramebuffer::FFramebuffer(VkDevice device)
    : m_Device(device)
    , m_Framebuffer(VK_NULL_HANDLE)
{
}

FFramebuffer::~FFramebuffer()
{
    if (m_Framebuffer != VK_NULL_HANDLE)
    {
        vkDestroyFramebuffer(m_Device, m_Framebuffer, nullptr);
        m_Framebuffer = VK_NULL_HANDLE;
    }

    m_Device = VK_NULL_HANDLE;
}
