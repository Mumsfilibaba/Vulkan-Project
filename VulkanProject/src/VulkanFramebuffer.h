#pragma once
#include "Core.h"

#include <vulkan/vulkan.h>

class VulkanRenderPass;

struct FramebufferParams
{
	VulkanRenderPass* pRenderPass = nullptr;
	VkImageView* pAttachMents = nullptr;
	uint32 AttachMentCount = 0;
	uint32 Width	= 0;
	uint32 Height	= 0;
};

class VulkanFramebuffer
{
public:
	DECL_NO_COPY(VulkanFramebuffer);

	VulkanFramebuffer(VkDevice device, const FramebufferParams& params);
	~VulkanFramebuffer();

	VkExtent2D GetExtent() const { return { m_Width, m_Height }; }
	VkFramebuffer GetFramebuffer() const { return m_Framebuffer; }
private:
	void Init(const FramebufferParams& params);
private:
	VkDevice m_Device;
	VkFramebuffer m_Framebuffer;
	uint32 m_Width;
	uint32 m_Height;
};

