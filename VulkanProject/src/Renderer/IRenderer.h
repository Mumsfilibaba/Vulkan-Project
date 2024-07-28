#pragma once
#include "IScene.h"
#include "Vulkan/Device.h"
#include "Vulkan/DescriptorPool.h"

class FDeviceMemoryAllocator;

struct IRenderer
{
    virtual void Init(FDevice* pDevice, FSwapchain* pSwapchain) = 0;
    virtual void Release() = 0;
    virtual void Tick(float DeltaTime) = 0;

    virtual void OnRenderUI() = 0;

    virtual IScene* GetScene() const = 0;

    virtual FDevice* GetDevice() const = 0;
    virtual FDeviceMemoryAllocator* GetDeviceAllocator() const = 0;
    virtual FDescriptorPool* GetDescriptorPool() const = 0;
};
