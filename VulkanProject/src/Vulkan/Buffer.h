#pragma once
#include "Core.h"
#include "DeviceMemoryAllocator.h"

#define VK_CPU_BUFFER_USAGE (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
#define VK_GPU_BUFFER_USAGE (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)

class Device;

struct BufferParams
{
    VkDeviceSize          Size             = 0;
    VkMemoryPropertyFlags MemoryProperties = 0;
    VkBufferUsageFlags    Usage            = 0;
};

class Buffer
{
public:
    static Buffer* Create(Device* pDevice, const BufferParams& params, DeviceMemoryAllocator* pAllocator);
    static Buffer* CreateWithData(Device* pDevice, const BufferParams& params, DeviceMemoryAllocator* pAllocator, const void* pSource);

    Buffer(Device* pDevice, DeviceMemoryAllocator* pAllocator);
    ~Buffer();
    
    void* Map();
    void FlushMappedMemoryRange();
    void Unmap();
    
    VkBuffer GetBuffer() const
    {
        return m_Buffer;
    }
    
    VkDeviceSize GetSize() const
    {
        return m_Size;
    }
    
private:
    DeviceMemoryAllocator* m_pAllocator;
    Device*         m_pDevice;
    VkBuffer               m_Buffer;
    VkDeviceMemory         m_DeviceMemory;
    VkDeviceSize           m_Size;
    DeviceAllocation     m_Allocation;
};
