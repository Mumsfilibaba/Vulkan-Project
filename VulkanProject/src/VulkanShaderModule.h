#pragma once
#include "Core.h"

#include <vulkan/vulkan.h>

class VulkanContext;

class VulkanShaderModule
{
public:
	DECL_NO_COPY(VulkanShaderModule);

	VulkanShaderModule(VkDevice device, const char* pEntryPoint, const char* pSource, uint32 length);
	~VulkanShaderModule();

	VkShaderModule GetModule() const { return m_Module; }
	const char* GetEntryPoint() const { return m_pEntryPoint; }

	static VulkanShaderModule* CreateFromFile(VulkanContext* pContext, const char* pEntryPoint, const char* pFilePath);
private:
	VkDevice m_Device;
	VkShaderModule m_Module;
	char* m_pEntryPoint;
};