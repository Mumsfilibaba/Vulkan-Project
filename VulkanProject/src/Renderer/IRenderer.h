#pragma once
#include "IScene.h"
#include "Vulkan/Device.h"
#include "Vulkan/DescriptorPool.h"

class CDeviceMemoryAllocator;

struct IRenderer
{
    virtual void Init(CDevice* pDevice, CSwapchain* pSwapchain) = 0;
    virtual void Release() = 0;
    virtual void Tick(float DeltaTime) = 0;

    virtual void OnRenderUI() = 0;

    virtual IScene* GetScene() const = 0;

    virtual CDevice* GetDevice() const = 0;
    virtual CDeviceMemoryAllocator* GetDeviceAllocator() const = 0;
    virtual CDescriptorPool* GetDescriptorPool() const = 0;
};
