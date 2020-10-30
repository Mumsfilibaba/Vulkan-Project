#pragma once
#include "Core.h"

#include <vulkan/vulkan.h>

class VulkanContext;

struct ShaderModuleParams
{
	const char* pEntryPoint = nullptr;
	const char* pSource = nullptr;
	uint32_t SourceSize = 0;
};

class VulkanShaderModule
{
public:
	VulkanShaderModule(VkDevice device, const ShaderModuleParams& params);
	~VulkanShaderModule();

	inline VkShaderModule GetModule() const
	{
		return m_Module;
	}
	
	inline const char* GetEntryPoint() const
	{
		return m_pEntryPoint;
	}

	static VulkanShaderModule* CreateFromFile(VulkanContext* pContext, const char* pEntryPoint, const char* pFilePath);
	
private:
	VkDevice m_Device;
	VkShaderModule m_Module;
	char* m_pEntryPoint;
};
