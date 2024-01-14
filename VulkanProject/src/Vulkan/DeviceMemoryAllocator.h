#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>
#include <vector>

struct FDeviceMemoryBlock;
struct FDeviceAllocation;
class FDeviceMemoryPage;

struct FDeviceMemoryBlock
{
    FDeviceMemoryPage*  pPage     = nullptr;
    FDeviceMemoryBlock* pNext     = nullptr;
    FDeviceMemoryBlock* pPrevious = nullptr;

    VkDeviceSize SizeInBytes        = 0;
    VkDeviceSize PaddedSizeInBytes  = 0;
    VkDeviceSize DeviceMemoryOffset = 0;
    bool         IsFree             = true;
    uint32_t     ID                 = 0;
};

struct FDeviceAllocation
{
    FDeviceMemoryBlock* pBlock             = nullptr;
    uint8_t*            pHostMemory        = nullptr;
    VkDeviceSize        SizeInBytes        = 0;
    VkDeviceSize        DeviceMemoryOffset = 0;
    VkDeviceMemory      DeviceMemory       = VK_NULL_HANDLE;
};

class FDeviceMemoryPage
{
public:
    FDeviceMemoryPage(VkDevice device, VkPhysicalDevice physicalDevice, const uint32_t id, VkDeviceSize sizeInBytes, uint32_t memoryType, VkMemoryPropertyFlags properties);
    ~FDeviceMemoryPage();

    bool Allocate(FDeviceAllocation& allocation, VkDeviceSize sizeInBytes, VkDeviceSize alignment, VkDeviceSize granularity);
    void Deallocate(FDeviceAllocation& allocation);

    bool IsEmpty() const
    {
        return m_pHead->pPrevious == nullptr && m_pHead->pNext == nullptr && m_pHead->IsFree;
    }
    
    uint64_t GetSizeInBytes() const
    {
        return m_SizeInBytes;
    }
    
    uint32_t GetMemoryType() const
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
    FDeviceMemoryBlock*   m_pHead;
    uint8_t*              m_pHostMemory;
    VkMemoryPropertyFlags m_Properties;
    const uint32_t        m_ID;
    const uint32_t        m_MemoryType;
    const uint64_t        m_SizeInBytes;
    uint32_t              m_BlockCount;
    bool                  m_IsMapped;
};

class FDeviceMemoryAllocator
{
public:
    FDeviceMemoryAllocator(VkDevice device, VkPhysicalDevice physicalDevice);
    ~FDeviceMemoryAllocator();

    bool Allocate(FDeviceAllocation& allocation, const VkMemoryRequirements& memoryRequirements, VkMemoryPropertyFlags properties);
    void Deallocate(FDeviceAllocation& allocation);
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
    VkDevice                                    m_Device;
    VkPhysicalDevice                            m_PhysicalDevice;
    VkDeviceSize                                m_BufferImageGranularity;
    std::vector<FDeviceMemoryPage*>             m_Pages;
    std::vector<std::vector<FDeviceAllocation>> m_GarbageMemory;
    uint64_t                                    m_FrameIndex;
    uint64_t                                    m_TotalAllocated;
    uint64_t                                    m_TotalReserved;
    uint64_t                                    m_MaxAllocations;
};

