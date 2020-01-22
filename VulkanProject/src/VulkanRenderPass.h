#pragma once
#include "Core.h"

#include <vulkan/vulkan.h>

struct RenderPassAttachment
{
	VkFormat Format = VK_FORMAT_UNDEFINED;
};

struct RenderPassParams
{
	RenderPassAttachment* pColorAttachments = nullptr;
	uint32 ColorAttachmentCount = 0;
};

class VulkanRenderPass
{
public:
	DECL_NO_COPY(VulkanRenderPass);

	VulkanRenderPass(VkDevice device, const RenderPassParams& params);
	~VulkanRenderPass();

	VkRenderPass GetRenderPass() const { return m_RenderPass; }
private:
	void Init(const RenderPassParams& params);
private:
	VkDevice m_Device;
	VkRenderPass m_RenderPass;
};