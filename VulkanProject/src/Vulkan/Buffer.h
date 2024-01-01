#pragma once
#include "Core.h"
#include "VulkanDeviceAllocator.h"

#define VK_CPU_BUFFER_USAGE (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
#define VK_GPU_BUFFER_USAGE (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)

struct BufferParams
{
    VkMemoryPropertyFlags MemoryProperties = 0;
    uint32_t              Usage            = 0;
    VkDeviceSize          SizeInBytes      = 0;
};

class Buffer
{
public:
    static Buffer* Create(class VulkanContext* pContext, const BufferParams& params, VulkanDeviceAllocator* pAllocator);

    Buffer(VkDevice device, VkPhysicalDevice physicalDevice, VulkanDeviceAllocator* pAllocator);
    ~Buffer();
    
    void* Map();

    void Unmap()
    {
        if (!m_pAllocator)
        {
            vkUnmapMemory(m_Device, m_DeviceMemory);
        }
    }

    
    VkBuffer GetBuffer() const
    {
        return m_Buffer;
    }
    
    
private:
    VulkanDeviceAllocator* m_pAllocator;
    VkDevice               m_Device;
    VkPhysicalDevice       m_PhysicalDevice;
    VkBuffer               m_Buffer;
    VkDeviceMemory         m_DeviceMemory;
    VkDeviceSize           m_SizeInBytes;
    VkDeviceAllocation     m_Allocation;
};
