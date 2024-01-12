#pragma once
#include "Core.h"
#include "DeviceMemoryAllocator.h"

#define VK_CPU_BUFFER_USAGE (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
#define VK_GPU_BUFFER_USAGE (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)

class FDevice;

struct FBufferParams
{
    VkDeviceSize          Size             = 0;
    VkMemoryPropertyFlags MemoryProperties = 0;
    VkBufferUsageFlags    Usage            = 0;
};

class FBuffer
{
public:
    static FBuffer* Create(FDevice* pDevice, const FBufferParams& params, FDeviceMemoryAllocator* pAllocator);
    static FBuffer* CreateWithData(FDevice* pDevice, const FBufferParams& params, FDeviceMemoryAllocator* pAllocator, const void* pSource);

    FBuffer(FDevice* pDevice, FDeviceMemoryAllocator* pAllocator);
    ~FBuffer();
    
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
    FDeviceMemoryAllocator* m_pAllocator;
    FDevice*                m_pDevice;
    VkBuffer                m_Buffer;
    VkDeviceMemory          m_DeviceMemory;
    VkDeviceSize            m_Size;
    FDeviceAllocation       m_Allocation;
};
