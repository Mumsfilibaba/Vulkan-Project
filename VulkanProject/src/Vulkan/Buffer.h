#pragma once
#include "DeviceChild.h"
#include "DeviceMemoryAllocator.h"

#define VK_CPU_BUFFER_USAGE (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
#define VK_GPU_BUFFER_USAGE (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)

#define VK_BUFFER_USAGE_RAY_TRACING_INPUT (VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR)

class CDevice;

struct SBufferParams
{
    VkDeviceSize          Size             = 0;
    VkMemoryPropertyFlags MemoryProperties = 0;
    VkBufferUsageFlags    Usage            = 0;
};

class CBuffer : public CDeviceChild
{
public:
    static CBuffer* Create(CDevice* pDevice, const SBufferParams& Params, CDeviceMemoryAllocator* pAllocator);
    static CBuffer* CreateWithData(CDevice* pDevice, const SBufferParams& Params, CDeviceMemoryAllocator* pAllocator, const void* pSource);
    static CBuffer* CreateAndCopy(CDevice* pDevice, const SBufferParams& Params, CDeviceMemoryAllocator* pAllocator, CBuffer* pSrcBuffer);

    CBuffer(CDevice* pDevice, CDeviceMemoryAllocator* pAllocator = nullptr);
    ~CBuffer();
    
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
    CDeviceMemoryAllocator*       m_pAllocator;
    VkBuffer                      m_Buffer;
    VkDeviceMemory                m_DeviceMemory;
    VkDeviceOrHostAddressConstKHR m_DeviceAddress;
    VkDeviceSize                  m_Size;
    VkDeviceSize                  m_AllocatedSize;
    SDeviceAllocation             m_Allocation;
};
