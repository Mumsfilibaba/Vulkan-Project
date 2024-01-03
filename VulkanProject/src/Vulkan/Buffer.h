#pragma once
#include "Core.h"
#include "VulkanDeviceAllocator.h"

#define VK_CPU_BUFFER_USAGE (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
#define VK_GPU_BUFFER_USAGE (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)

class VulkanContext;

struct BufferParams
{
    VkDeviceSize          Size             = 0;
    VkMemoryPropertyFlags MemoryProperties = 0;
    VkBufferUsageFlags    Usage            = 0;
};

class Buffer
{
public:
    static Buffer* Create(VulkanContext* pContext, const BufferParams& params, VulkanDeviceAllocator* pAllocator);
    static Buffer* CreateWithData(VulkanContext* pContext, const BufferParams& params, VulkanDeviceAllocator* pAllocator, const void* pSource);

    Buffer(VulkanContext* pContext, VulkanDeviceAllocator* pAllocator);
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
    VulkanDeviceAllocator* m_pAllocator;
    VulkanContext*         m_pContext;
    VkBuffer               m_Buffer;
    VkDeviceMemory         m_DeviceMemory;
    VkDeviceSize           m_Size;
    VkDeviceAllocation     m_Allocation;
};
