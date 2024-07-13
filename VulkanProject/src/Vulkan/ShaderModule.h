#pragma once
#include "DeviceChild.h"
#include <vulkan/vulkan.h>

class FDevice;

class FShaderModule : public FDeviceChild
{
public:
    static FShaderModule* Create(FDevice* pDevice, const uint32_t* pByteCode, uint32_t ByteCodeLength, const char* pEntryPoint);
    static FShaderModule* CreateFromFile(FDevice* pDevice, const char* pEntryPoint, const char* pFilePath);
    
    FShaderModule(FDevice* pDevice);
    ~FShaderModule();

    void SetDebugName(const char* DebugName);
    
    VkShaderModule GetModule() const
    {
        return m_Module;
    }
    
    const char* GetEntryPoint() const
    {
        return m_pEntryPoint;
    }

private:
    VkShaderModule m_Module;
    char*          m_pEntryPoint;
};
