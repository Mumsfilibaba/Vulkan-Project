#pragma once
#include "Vulkan/VulkanContext.h"
#include "Vulkan/DescriptorPool.h"

class IRenderer
{
public:
    virtual void Init(VulkanContext* pContext) = 0;
    virtual void Release() = 0;
    
    // dt is in seconds
    virtual void Tick(float dt) = 0;
    
    virtual void OnWindowResize(uint32_t width, uint32_t height) = 0;
};
