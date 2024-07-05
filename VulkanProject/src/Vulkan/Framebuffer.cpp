#include "Framebuffer.h"
#include "RenderPass.h"
#include "Device.h"

FFramebuffer* FFramebuffer::Create(FDevice* pDevice, const FFramebufferParams& Params)
{
    FFramebuffer* pFramebuffer = new FFramebuffer(pDevice);
    
    assert(Params.pRenderPass != nullptr);

    VkFramebufferCreateInfo FramebufferCreateInfo;
    ZERO_STRUCT(&FramebufferCreateInfo);
    
    FramebufferCreateInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FramebufferCreateInfo.renderPass      = Params.pRenderPass->GetRenderPass();
    FramebufferCreateInfo.attachmentCount = Params.AttachMentCount;
    FramebufferCreateInfo.pAttachments    = Params.pAttachMents;
    FramebufferCreateInfo.width           = Params.Width;
    FramebufferCreateInfo.height          = Params.Height;
    FramebufferCreateInfo.layers          = 1;

    VkResult Result = vkCreateFramebuffer(pDevice->GetDevice(), &FramebufferCreateInfo, nullptr, &pFramebuffer->m_Framebuffer);
    if (Result != VK_SUCCESS)
    {
        std::cout << "vkCreateFramebuffer failed\n";
        return nullptr;
    }
    else
    {
        std::cout << "Created Framebuffer\n";

        pFramebuffer->m_Width  = Params.Width;
        pFramebuffer->m_Height = Params.Height;
    }
    
    return pFramebuffer;
}

FFramebuffer::FFramebuffer(FDevice* pDevice)
    : FDeviceChild(pDevice)
    , m_Framebuffer(VK_NULL_HANDLE)
{
}

FFramebuffer::~FFramebuffer()
{
    if (m_Framebuffer != VK_NULL_HANDLE)
    {
        vkDestroyFramebuffer(GetDevice()->GetDevice(), m_Framebuffer, nullptr);
        m_Framebuffer = VK_NULL_HANDLE;
    }
}
