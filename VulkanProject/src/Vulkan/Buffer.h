#pragma once
#include "DeviceChild.h"
#include "DeviceMemoryAllocator.h"

#define VK_CPU_BUFFER_USAGE (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
#define VK_GPU_BUFFER_USAGE (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)

#define VK_BUFFER_USAGE_RAY_TRACING_INPUT (VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR)

class FDevice;

struct FBufferParams
{
    VkDeviceSize          Size             = 0;
    VkMemoryPropertyFlags MemoryProperties = 0;
    VkBufferUsageFlags    Usage            = 0;
};

class FBuffer : public FDeviceChild
{
public:
    static FBuffer* Create(FDevice* pDevice, const FBufferParams& Params, FDeviceMemoryAllocator* pAllocator);
    static FBuffer* CreateWithData(FDevice* pDevice, const FBufferParams& Params, FDeviceMemoryAllocator* pAllocator, const void* pSource);

    FBuffer(FDevice* pDevice, FDeviceMemoryAllocator* pAllocator = nullptr);
    ~FBuffer();
    
    void* Map();
    void FlushMappedMemoryRange();
    void Unmap();
    void SetDebugName(const char* DebugName);
    
    VkBuffer GetBuffer() const
    {
        return m_Buffer;
    }
    
    VkDeviceSize GetSize() const
    {
        return m_Size;
    }

    VkDeviceOrHostAddressConstKHR GetDeviceAddress() const
    {
        return m_DeviceAddress;
    }

private:
    FDeviceMemoryAllocator*       m_pAllocator;
    VkBuffer                      m_Buffer;
    VkDeviceMemory                m_DeviceMemory;
    VkDeviceOrHostAddressConstKHR m_DeviceAddress;
    VkDeviceSize                  m_Size;
    VkDeviceSize                  m_AllocatedSize;
    FDeviceAllocation             m_Allocation;
};
