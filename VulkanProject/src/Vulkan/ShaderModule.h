#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>

class FDevice;

class FShaderModule
{
public:
    static FShaderModule* Create(FDevice* pDevice, const uint32_t* pByteCode, uint32_t byteCodeLength, const char* pEntryPoint);
    static FShaderModule* CreateFromFile(FDevice* pDevice, const char* pEntryPoint, const char* pFilePath);
    
    FShaderModule(VkDevice device);
    ~FShaderModule();

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
