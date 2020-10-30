#pragma once
#include "Core.h"

#include <vulkan/vulkan.h>

class VulkanRenderPass;

struct FramebufferParams
{
	VulkanRenderPass* pRenderPass = nullptr;
	VkImageView* pAttachMents = nullptr;
	uint32_t AttachMentCount = 0;
	uint32_t Width	= 0;
	uint32_t Height	= 0;
};

class VulkanFramebuffer
{
public:
	VulkanFramebuffer(VkDevice device, const FramebufferParams& params);
	~VulkanFramebuffer();

	inline VkExtent2D GetExtent() const
	{
		return { m_Width, m_Height };
	}
	
	inline VkFramebuffer GetFramebuffer() const
	{
		return m_Framebuffer;
	}

private:
	void Init(const FramebufferParams& params);
	
private:
	VkDevice m_Device;
	VkFramebuffer m_Framebuffer;
	uint32_t m_Width;
	uint32_t m_Height;
};

