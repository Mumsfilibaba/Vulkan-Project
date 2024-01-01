#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>
#include <vector>

struct VkMemoryBlock;
struct VkDeviceAllocation;
class VkMemoryPage;

struct VkMemoryBlock
{
    VkMemoryPage*  pPage              = nullptr;
    VkMemoryBlock* pNext              = nullptr;
    VkMemoryBlock* pPrevious          = nullptr;
    VkDeviceSize   SizeInBytes        = 0;
    VkDeviceSize   PaddedSizeInBytes  = 0;
    VkDeviceSize   DeviceMemoryOffset = 0;
    bool           IsFree             = true;
    uint32_t       ID                 = 0;
};

struct VkDeviceAllocation
{
    VkMemoryBlock* pBlock             = nullptr;
    uint8_t*       pHostMemory        = nullptr;
    VkDeviceSize   SizeInBytes        = 0;
    VkDeviceSize   DeviceMemoryOffset = 0;
    VkDeviceMemory DeviceMemory       = VK_NULL_HANDLE;
};

class VkMemoryPage
{
public:
    VkMemoryPage(VkDevice device, VkPhysicalDevice physicalDevice, const uint32_t id, VkDeviceSize sizeInBytes, uint32_t memoryType, VkMemoryPropertyFlags properties);
    ~VkMemoryPage();

    bool Allocate(VkDeviceAllocation& allocation, VkDeviceSize sizeInBytes, VkDeviceSize alignment, VkDeviceSize granularity);
    void Deallocate(VkDeviceAllocation& allocation);

    bool IsEmpty() const
    {
        return m_pHead->pPrevious == nullptr && m_pHead->pNext == nullptr && m_pHead->IsFree;
    }
    
    inline uint64_t GetSizeInBytes() const
    {
        return m_SizeInBytes;
    }
    
    inline uint32_t GetMemoryType() const
    {
        return m_MemoryType;
    }
    
private:
    bool IsOnSamePage(VkDeviceSize aOffset, VkDeviceSize aSize, VkDeviceSize bOffset, VkDeviceSize pageSize);
    void Init();
    void Map();
    void Unmap();
    
    VkDevice              m_Device;
    VkPhysicalDevice      m_PhysicalDevice;
    VkDeviceMemory        m_DeviceMemory;
    VkMemoryBlock*        m_pHead;
    uint8_t*              m_pHostMemory;
    VkMemoryPropertyFlags m_Properties;
    const uint32_t        m_ID;
    const uint32_t        m_MemoryType;
    const uint64_t        m_SizeInBytes;
    uint32_t              m_BlockCount;
    bool                  m_IsMapped;
};

class VulkanDeviceAllocator
{
public:
    VulkanDeviceAllocator(VkDevice device, VkPhysicalDevice physicalDevice);
    ~VulkanDeviceAllocator();

    bool Allocate(VkDeviceAllocation& allocation, const VkMemoryRequirements& memoryRequirements, VkMemoryPropertyFlags properties);
    void Deallocate(VkDeviceAllocation& allocation);
    void EmptyGarbageMemory();

    uint64_t GetTotalReserved() const
    {
        return m_TotalReserved;
    }
    
    uint64_t GetTotalAllocated() const
    {
        return m_TotalAllocated;
    }
    
private:
    VkDevice                                     m_Device;
    VkPhysicalDevice                             m_PhysicalDevice;
    VkDeviceSize                                 m_BufferImageGranularity;
    std::vector<VkMemoryPage*>                   m_Pages;
    std::vector<std::vector<VkDeviceAllocation>> m_GarbageMemory;
    uint64_t                                     m_FrameIndex;
    uint64_t                                     m_TotalAllocated;
    uint64_t                                     m_TotalReserved;
    uint64_t                                     m_MaxAllocations;
};

