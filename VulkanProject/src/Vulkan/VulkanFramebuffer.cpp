#include "VulkanFramebuffer.h"
#include "VulkanRenderPass.h"

VulkanFramebuffer::VulkanFramebuffer(VkDevice device, const FramebufferParams& params)
	: m_Device(device),
	m_Framebuffer(VK_NULL_HANDLE)
{
	Init(params);
}

VulkanFramebuffer::~VulkanFramebuffer()
{
	if (m_Framebuffer != VK_NULL_HANDLE)
	{
		vkDestroyFramebuffer(m_Device, m_Framebuffer, nullptr);
		m_Framebuffer = VK_NULL_HANDLE;

        std::cout << "Destroyed Framebuffer" << std::endl;
	}
}

void VulkanFramebuffer::Init(const FramebufferParams& params)
{
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

    VkResult result = vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_Framebuffer);
    if (result != VK_SUCCESS) 
    {
        std::cout << "vkCreateFramebuffer failed" << std::endl;
    }
    else
    {
        std::cout << "Created framebuffer" << std::endl;

        m_Width  = params.Width;
        m_Height = params.Height;
    }
}
