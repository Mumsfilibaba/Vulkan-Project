#pragma once
#include "Vulkan/Device.h"
#include "Vulkan/DescriptorPool.h"

class IRenderer
{
public:
    virtual void Init(Device* pDevice, Swapchain* pSwapchain) = 0;
    
    virtual void Release() = 0;
    
    virtual void Tick(float deltaTime) = 0;
    
    virtual void OnRenderUI() = 0;
    
    virtual void OnWindowResize(uint32_t width, uint32_t height) = 0;
};
