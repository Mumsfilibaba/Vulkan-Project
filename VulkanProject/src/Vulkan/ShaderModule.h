#pragma once
#include "DeviceChild.h"
#include <vulkan/vulkan.h>

class CDevice;

class CShaderModule : public CDeviceChild
{
public:
    static CShaderModule* Create(CDevice* pDevice, const uint32_t* pByteCode, uint32_t ByteCodeLength, const char* pEntryPoint);
    static CShaderModule* CreateFromFile(CDevice* pDevice, const char* pEntryPoint, const char* pFilePath);
    
    CShaderModule(CDevice* pDevice);
    ~CShaderModule();

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
