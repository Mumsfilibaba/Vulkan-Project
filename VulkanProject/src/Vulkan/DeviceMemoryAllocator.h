#pragma once
#include "DeviceChild.h"
#include <vulkan/vulkan.h>

struct SDeviceMemoryBlock;
struct SDeviceAllocation;
class CDeviceMemoryPage;

struct SDeviceMemoryBlock
{
    CDeviceMemoryPage*  pPage     = nullptr;
    SDeviceMemoryBlock* pNext     = nullptr;
    SDeviceMemoryBlock* pPrevious = nullptr;

    VkDeviceSize SizeInBytes        = 0;
    VkDeviceSize PaddedSizeInBytes  = 0;
    VkDeviceSize DeviceMemoryOffset = 0;
    bool         IsFree             = true;
    uint32_t     ID                 = 0;
};

struct SDeviceAllocation
{
    SDeviceMemoryBlock* pBlock             = nullptr;
    uint8_t*            pHostMemory        = nullptr;
    VkDeviceSize        SizeInBytes        = 0;
    VkDeviceSize        DeviceMemoryOffset = 0;
    VkDeviceMemory      DeviceMemory       = VK_NULL_HANDLE;
};

class CDeviceMemoryPage
{
public:
    CDeviceMemoryPage(VkDevice Device, VkPhysicalDevice PhysicalDevice, const uint32_t Id, VkDeviceSize SizeInBytes, uint32_t MemoryType, VkMemoryPropertyFlags Properties);
    ~CDeviceMemoryPage();

    bool Allocate(SDeviceAllocation& Allocation, VkDeviceSize SizeInBytes, VkDeviceSize Slignment, VkDeviceSize Granularity);
    void Deallocate(SDeviceAllocation& Allocation);

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
    bool IsOnSamePage(VkDeviceSize OffsetA, VkDeviceSize SizeA, VkDeviceSize OffsetB, VkDeviceSize PageSize);
    void Init();
    void Map();
    void Unmap();
    
    VkDevice              m_Device;
    VkPhysicalDevice      m_PhysicalDevice;
    VkDeviceMemory        m_DeviceMemory;
    SDeviceMemoryBlock*   m_pHead;
    uint8_t*              m_pHostMemory;
    VkMemoryPropertyFlags m_Properties;
    const uint32_t        m_ID;
    const uint32_t        m_MemoryType;
    const uint64_t        m_SizeInBytes;
    uint32_t              m_BlockCount;
    bool                  m_IsMapped;
};

class CDeviceMemoryAllocator : public CDeviceChild
{
public:
    CDeviceMemoryAllocator(CDevice* pDevice);
    ~CDeviceMemoryAllocator();

    bool Allocate(SDeviceAllocation& Allocation, const VkMemoryRequirements& MemoryRequirements, VkMemoryPropertyFlags Properties);
    void Deallocate(SDeviceAllocation& Allocation);
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
    VkDeviceSize                                m_BufferImageGranularity;
    std::vector<CDeviceMemoryPage*>             m_Pages;
    std::vector<std::vector<SDeviceAllocation>> m_GarbageMemory;
    uint64_t                                    m_FrameIndex;
    uint64_t                                    m_TotalAllocated;
    uint64_t                                    m_TotalReserved;
    uint64_t                                    m_MaxAllocations;
};

