#pragma once
#include "Core.h"

#include <vulkan/vulkan.h>

class VulkanContext;

class ShaderModule
{
public:
    ShaderModule(VkDevice device);
    ~ShaderModule();

    inline VkShaderModule GetModule() const
    {
        return m_Module;
    }
    
    inline const char* GetEntryPoint() const
    {
        return m_pEntryPoint;
    }

    static ShaderModule* CreateFromFile(VulkanContext* pContext, const char* pEntryPoint, const char* pFilePath);
    
private:
    VkDevice m_Device;
    VkShaderModule m_Module;
    char* m_pEntryPoint;
};
