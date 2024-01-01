#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>

class VulkanContext;

class ShaderModule
{
public:
    static ShaderModule* CreateFromFile(VulkanContext* pContext, const char* pEntryPoint, const char* pFilePath);
    
    ShaderModule(VkDevice device);
    ~ShaderModule();

    VkShaderModule GetModule() const
    {
        return m_Module;
    }
    
    const char* GetEntryPoint() const
    {
        return m_pEntryPoint;
    }

private:
    VkDevice       m_Device;
    VkShaderModule m_Module;
    char*          m_pEntryPoint;
};
