#include "Framebuffer.h"
#include "RenderPass.h"
#include "VulkanContext.h"

Framebuffer::Framebuffer(VkDevice device)
	: m_Device(device),
	m_Framebuffer(VK_NULL_HANDLE)
{
}

Framebuffer::~Framebuffer()
{
	if (m_Framebuffer != VK_NULL_HANDLE)
	{
		vkDestroyFramebuffer(m_Device, m_Framebuffer, nullptr);
		m_Framebuffer = VK_NULL_HANDLE;

        std::cout << "Destroyed Framebuffer" << std::endl;
	}
}

Framebuffer* Framebuffer::Create(VulkanContext* pContext, const FramebufferParams& params)
{
	Framebuffer* newFrameBuffer = new Framebuffer(pContext->GetDevice());
	
    assert(params.pRenderPass != nullptr);

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.flags           = 0;
    framebufferInfo.pNext           = nullptr;
    framebufferInfo.renderPass      = params.pRenderPass->GetRenderPass();
    framebufferInfo.attachmentCount = params.AttachMentCount;
    framebufferInfo.pAttachments    = params.pAttachMents;
    framebufferInfo.width           = params.Width;
    framebufferInfo.height          = params.Height;
    framebufferInfo.layers          = 1;

	VkResult result = vkCreateFramebuffer(newFrameBuffer->m_Device, &framebufferInfo, nullptr, &newFrameBuffer->m_Framebuffer);
    if (result != VK_SUCCESS) 
    {
        std::cout << "vkCreateFramebuffer failed" << std::endl;
		return nullptr;
    }
    else
    {
        std::cout << "Created framebuffer" << std::endl;

		newFrameBuffer->m_Width  = params.Width;
		newFrameBuffer->m_Height = params.Height;
    }
	
	return newFrameBuffer;
}
