#pragma once
#include "Vulkan/Device.h"
#include "Vulkan/DescriptorPool.h"

struct IRenderer
{
    virtual void Init(FDevice* pDevice, FSwapchain* pSwapchain) = 0;
    virtual void Release() = 0;
    virtual void Tick(float DeltaTime) = 0;
    
    virtual void OnRenderUI() = 0;
    virtual void OnWindowResize(uint32_t Width, uint32_t Height) = 0;
};
