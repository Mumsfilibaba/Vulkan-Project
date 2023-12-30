#include "VulkanDeviceAllocator.h"
#include "VulkanHelper.h"
#include "MathHelper.h"

#include <assert.h>

//#define ALLOCATOR_DEBUG
#define MB(bytes) bytes * 1024 * 1024

constexpr float mb = 1024.0f * 1024.0f;

VkMemoryPage::VkMemoryPage(VkDevice device, VkPhysicalDevice phyicalDevice, uint32_t id, VkDeviceSize sizeInBytes, uint32_t memoryType, VkMemoryPropertyFlags properties)
    : m_Device(device),
    m_PhysicalDevice(phyicalDevice),
    m_Properties(properties),
    m_ID(id),
    m_MemoryType(memoryType),
    m_SizeInBytes(sizeInBytes),
    m_BlockCount(0),
    m_pHead(nullptr),
    m_pHostMemory(nullptr),
    m_IsMapped(false)
{
    Init();
}


VkMemoryPage::~VkMemoryPage()
{
    if (m_DeviceMemory != VK_NULL_HANDLE)
    {
        //Unmap
        Unmap();

        //Print memoryleaks
#if defined(ALLOCATOR_DEBUG)
        {
            VkMemoryBlock* pDebug = m_pHead;
            std::cout << "Allocated blocks left in MemoryPage '" << m_ID << "'" << std::endl;
            while (pDebug)
            {
                std::cout << "    Block: ID=" << pDebug->ID << ", Offset=" << pDebug->DeviceMemoryOffset << ", Size=" << pDebug->SizeInBytes << ", IsFree=" << std::boolalpha << pDebug->IsFree << std::endl;
                pDebug = pDebug->pNext;
            }
        }
#endif

        //Delete all blocks
        VkMemoryBlock* pCurrent = m_pHead;
        while (pCurrent != nullptr)
        {
            VkMemoryBlock* pOld = pCurrent;
            pCurrent = pCurrent->pNext;

            //Delete block
            delete pOld;
        }

        //Free memory
        vkFreeMemory(m_Device, m_DeviceMemory, nullptr);
        m_DeviceMemory = VK_NULL_HANDLE;

        std::cout << "Deallocated MemoryPage" << std::endl;
    }
}

void VkMemoryPage::Init()
{
    //Allocate device memory
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.allocationSize    = m_SizeInBytes;
    allocInfo.memoryTypeIndex    = m_MemoryType;

    if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_DeviceMemory) != VK_SUCCESS)
    {
        std::cout << "vkAllocateMemory failed" << std::endl;
        return;
    }
    else
    {
        std::cout << "Allocated '" << m_SizeInBytes << "' bytes for MemoryPage" << std::endl;
    }

    //Setup first block
    m_pHead = new VkMemoryBlock();
    m_pHead->pPage = this;
    m_pHead->pNext = nullptr;
    m_pHead->pPrevious = nullptr;
    m_pHead->IsFree = true;
    m_pHead->ID = m_BlockCount++;
    m_pHead->SizeInBytes = m_SizeInBytes;
    m_pHead->PaddedSizeInBytes = m_SizeInBytes;
    m_pHead->DeviceMemoryOffset = 0;

    //If this is CPU visible -> Map
    if (m_Properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        Map();
    }
}


bool VkMemoryPage::Allocate(VkDeviceAllocation& allocation, VkDeviceSize sizeInBytes, VkDeviceSize alignment, VkDeviceSize granularity)
{
    VkDeviceSize paddedDeviceOffset = 0;
    VkDeviceSize paddedSizeInBytes = 0;
    VkMemoryBlock* pBestFit = nullptr;

    //Find enough free space, and find the block that best fits
    for (VkMemoryBlock* pCurrent = m_pHead; pCurrent != nullptr; pCurrent = pCurrent->pNext)
    {
        //Check if the block is allocated or not
        if (!pCurrent->IsFree)
            continue;

        //Does it fit into the block
        if (sizeInBytes > pCurrent->SizeInBytes)
            continue;

        //Align the offset
        paddedDeviceOffset = Math::AlignUp<uint64_t>(pCurrent->DeviceMemoryOffset, alignment);

        //Take granularity into account
        if (pCurrent->pPrevious != nullptr && granularity > 1)
        {
            VkMemoryBlock* pPrevious = pCurrent->pPrevious;
            if (IsOnSamePage(pPrevious->DeviceMemoryOffset, pPrevious->SizeInBytes, paddedDeviceOffset, granularity))
            {
                paddedDeviceOffset = Math::AlignUp(paddedDeviceOffset, granularity);
            }
        }

        //Calculate padding
        paddedSizeInBytes = sizeInBytes + (paddedDeviceOffset - pCurrent->DeviceMemoryOffset);

        //Does it still fit
        if (paddedSizeInBytes > pCurrent->SizeInBytes)
            continue;

        //Avoid granularity conflict
        if (granularity > 1 && pCurrent->pNext != nullptr)
        {
            VkMemoryBlock* pNext = pCurrent->pNext;
            if (IsOnSamePage(paddedDeviceOffset, sizeInBytes, pNext->DeviceMemoryOffset, granularity))
            {
                continue;
            }
        }

        pBestFit = pCurrent;
        break;
    }

    //Did we find a suitable block to make the allocation?
    if (pBestFit == nullptr)
    {
        return false;
    }


    //        Free block
    //|--------------------------|
    //padding Allocation Remaining
    //|------|----------|--------|
    if (pBestFit->SizeInBytes > paddedSizeInBytes)
    {
        //Create a new block after allocation
        VkMemoryBlock* pBlock = new VkMemoryBlock();
        pBlock->pPage = this;
        pBlock->ID = m_BlockCount++;
        pBlock->SizeInBytes = pBestFit->SizeInBytes - paddedSizeInBytes;
        pBlock->PaddedSizeInBytes = pBlock->SizeInBytes;
        pBlock->DeviceMemoryOffset = pBestFit->DeviceMemoryOffset + paddedSizeInBytes;
        pBlock->IsFree = true;

        //Set pointers
        pBlock->pNext = pBestFit->pNext;
        pBlock->pPrevious = pBestFit;
        if (pBestFit->pNext)
            pBestFit->pNext->pPrevious = pBlock;
        pBestFit->pNext = pBlock;
    }

    //Update bestfit
    pBestFit->SizeInBytes        = sizeInBytes;
    pBestFit->PaddedSizeInBytes = paddedSizeInBytes;
    pBestFit->IsFree = false;

    //Setup allocation
    allocation.pBlock = pBestFit;
    allocation.DeviceMemory = m_DeviceMemory;
    allocation.DeviceMemoryOffset = paddedDeviceOffset;
    allocation.SizeInBytes = sizeInBytes;
    if (m_Properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        allocation.pHostMemory = m_pHostMemory + allocation.DeviceMemoryOffset;
    else
        allocation.pHostMemory = nullptr;

#if defined (ALLOCATOR_DEBUG)
    {
        std::cout << "Memory Page '" << m_ID << "'" << std::endl;
        for (VkMemoryBlock* pCurrent = m_pHead; pCurrent != nullptr; pCurrent = pCurrent->pNext)
        {
            std::cout << "----Block " << pCurrent->ID << "----" << std::endl;
            std::cout << "Starts at: Dec=" << pCurrent->DeviceMemoryOffset << ", Hex=" << std::hex << pCurrent->DeviceMemoryOffset << std::dec << std::endl;
            std::cout << "Free: " << std::boolalpha << pCurrent->IsFree << std::endl;
            
            if (pCurrent->pPrevious)
            {
                VkMemoryBlock* pPrevious = pCurrent->pPrevious;
                if ((pPrevious->DeviceMemoryOffset + pPrevious->PaddedSizeInBytes) > pCurrent->DeviceMemoryOffset)
                {
                    std::cout << "Overlapping memory in page '" << m_ID << "' between blocks '" << pPrevious->ID << "' and '" << pCurrent->ID << std::endl;
                }
            }

            std::cout << "Padding=" << pCurrent->PaddedSizeInBytes - pCurrent->SizeInBytes << std::endl;
            std::cout << "SizeInBytes=" << pCurrent->PaddedSizeInBytes << std::endl;

            VkDeviceSize end = pCurrent->DeviceMemoryOffset + pCurrent->PaddedSizeInBytes;
            std::cout << "End at: Dec=" << end << ", Hex=" << std::hex << end << std::dec << std::endl;
            std::cout << "----------------" << std::endl;
        }
    }
#endif

    return true;
}


bool VkMemoryPage::IsOnSamePage(VkDeviceSize aOffset, VkDeviceSize aSize, VkDeviceSize bOffset, VkDeviceSize pageSize)
{
    assert(aOffset + aSize <= bOffset && aSize > 0 && pageSize > 0);

    VkDeviceSize aEnd = aOffset + (aSize - 1);
    VkDeviceSize aEndPage = aEnd & ~(pageSize - 1);
    VkDeviceSize bStart = bOffset;
    VkDeviceSize bStartPage = bStart & ~(pageSize - 1);
    return aEndPage == bStartPage;
}


void VkMemoryPage::Map()
{
    //If not mapped -> map
    if (!m_IsMapped)
    {
        void* pMemory = nullptr;
        vkMapMemory(m_Device, m_DeviceMemory, 0, VK_WHOLE_SIZE, 0, &pMemory);

        m_pHostMemory = reinterpret_cast<uint8_t*>(pMemory);
        m_IsMapped = true;
    }
}


void VkMemoryPage::Unmap()
{
    //If mapped -> unmap
    if (m_IsMapped)
    {
        vkUnmapMemory(m_Device, m_DeviceMemory);
        m_pHostMemory = nullptr;
        m_IsMapped = false;
    }
}


void VkMemoryPage::Deallocate(VkDeviceAllocation& allocation)
{

    //Try to find the correct block
    VkMemoryBlock* pCurrent = allocation.pBlock;
    if (!pCurrent)
    {
        std::cout << "Block owning allocation was not found" << std::endl;
        return;
    }

    std::cout << "Deallocating Block ID=" << pCurrent->ID << std::endl;

    //Set this block to free
    pCurrent->IsFree = true;

    //Merge previous with current
    if (pCurrent->pPrevious)
    {
        VkMemoryBlock* pPrevious = pCurrent->pPrevious;
        if (pPrevious->IsFree)
        {
            //Set size
            pPrevious->SizeInBytes         += pCurrent->PaddedSizeInBytes;
            pPrevious->PaddedSizeInBytes += pCurrent->PaddedSizeInBytes;

            //Set pointers
            pPrevious->pNext = pCurrent->pNext;
            if (pCurrent->pNext)
                pCurrent->pNext->pPrevious = pPrevious;

            //Remove block
            delete pCurrent;
            pCurrent = pPrevious;
        }
    }

    //Try and merge current with next
    if (pCurrent->pNext)
    {
        VkMemoryBlock* pNext = pCurrent->pNext;
        if (pNext->IsFree)
        {
            //Set size
            pCurrent->SizeInBytes        += pNext->PaddedSizeInBytes;
            pCurrent->PaddedSizeInBytes += pNext->PaddedSizeInBytes;

            //Set pointers
            if (pNext->pNext)
                pNext->pNext->pPrevious = pCurrent;
            pCurrent->pNext = pNext->pNext;

            //Remove block
            delete pNext;
        }
    }
}

constexpr size_t numFrames = 3;

VulkanDeviceAllocator::VulkanDeviceAllocator(VkDevice device, VkPhysicalDevice physicalDevice)
    : m_Device(device),
    m_PhysicalDevice(physicalDevice),
    m_MaxAllocations(0),
    m_TotalReserved(0),
    m_TotalAllocated(0),
    m_FrameIndex(0),
    m_Pages(),
    m_GarbageMemory()
{
    //Resize the number of garbage memory vectors
    m_GarbageMemory.resize(numFrames);

    //Setup from properties of the device
    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);

    m_MaxAllocations = properties.limits.maxMemoryAllocationCount;
    m_BufferImageGranularity = properties.limits.bufferImageGranularity;
}


VulkanDeviceAllocator::~VulkanDeviceAllocator()
{
    //Cleanup all garbage memory before deleting
    for (uint32_t i = 0; i < numFrames; i++)
        EmptyGarbageMemory();

    //Delete allocator
    std::cout << "Deleting DeviceAllocator. Number of Pages: " << m_Pages.size() << std::endl;
    for (VkMemoryPage* page : m_Pages)
        delete page;

    std::cout << "Destroyed DeviceAllocator" << std::endl;
}


bool VulkanDeviceAllocator::Allocate(VkDeviceAllocation& allocation, const VkMemoryRequirements& memoryRequirements, VkMemoryPropertyFlags properties)
{
    m_TotalAllocated += memoryRequirements.size;
    uint32_t memoryType = FindMemoryType(m_PhysicalDevice, memoryRequirements.memoryTypeBits, properties);

    //Try allocating from existing page
    for (auto page : m_Pages)
    {
        if (page->GetMemoryType() == memoryType)
        {
            if (page->Allocate(allocation, memoryRequirements.size, memoryRequirements.alignment, m_BufferImageGranularity))
            {
                std::cout << "Allocated '" << memoryRequirements.size << "' bytes. Memory-Type=" << memoryType  << ", Total Allocated: " << float(m_TotalAllocated) / mb <<  "  MB. Total Reserved " << float(m_TotalReserved) / mb << " MB" << std::endl;
                return true;
            }
        }
    }

    assert(m_Pages.size() < m_MaxAllocations);

    //If allocated is large, make a dedicated allocation
    uint64_t bytesToReserve = MB(128);
    if (memoryRequirements.size > bytesToReserve)
        bytesToReserve = memoryRequirements.size;

    //Add to total
    m_TotalReserved += bytesToReserve;

    //Allocate new page
    VkMemoryPage* pPage = new VkMemoryPage(m_Device, m_PhysicalDevice, uint32_t(m_Pages.size()), bytesToReserve, memoryType, properties);
    m_Pages.emplace_back(pPage);

    std::cout << "Allocated Memory-Page. Allocationcount: ' " << m_Pages.size() << "/" << m_MaxAllocations << "'. Memory-Type=" << memoryType << ". Total Allocated: " << float(m_TotalAllocated) / mb << " MB. Total Reserved " << float(m_TotalReserved) / mb << " MB"<< std::endl;

    return pPage->Allocate(allocation, memoryRequirements.size, memoryRequirements.alignment, m_BufferImageGranularity);
}


void VulkanDeviceAllocator::Deallocate(VkDeviceAllocation& allocation)
{
    //Set it to be removed
    if (allocation.pBlock && allocation.DeviceMemory != VK_NULL_HANDLE)
        m_GarbageMemory[m_FrameIndex].emplace_back(allocation);

    //Invalidate memory
    allocation.pBlock = nullptr;
    allocation.DeviceMemoryOffset = 0;
    allocation.SizeInBytes = 0;
    allocation.DeviceMemory = VK_NULL_HANDLE;
    allocation.pHostMemory = nullptr;
}


void VulkanDeviceAllocator::EmptyGarbageMemory()
{
    //Move on a frame
    m_FrameIndex = (m_FrameIndex + 1) % numFrames;

    //Clean memory
    auto& memoryBlocks = m_GarbageMemory[m_FrameIndex];
    if (!memoryBlocks.empty())
    {
        //Deallocate all the blocks
        for (auto& memory : memoryBlocks)
        {
            if (memory.pBlock && memory.DeviceMemory != VK_NULL_HANDLE)
            {
                VkMemoryPage* pPage = memory.pBlock->pPage;
                pPage->Deallocate(memory);

                m_TotalAllocated -= memory.SizeInBytes;

                std::cout << "Deallocated '" << memory.SizeInBytes << "' bytes. Total Allocated: " << float(m_TotalAllocated) / mb << " MB. Total Reserved " << float(m_TotalReserved) / mb << " MB" << std::endl;
            }
        }

        memoryBlocks.clear();
    }


    //Remove empty pages
    if (m_Pages.size() > 6)
    {
        for (auto it = m_Pages.begin(); it != m_Pages.end();)
        {
            if ((*it)->IsEmpty())
            {
                //Erase from vector
                m_TotalReserved -= (*it)->GetSizeInBytes();

                delete *it;
                it = m_Pages.erase(it);

                std::cout << "Destroyed Memory-Page. Allocationcount: '" << m_Pages.size() << "/" << m_MaxAllocations << "'. Total Allocated: " << float(m_TotalAllocated) / mb << " MB. Total Reserved " << float(m_TotalReserved) / mb << " MB" << std::endl;
            }
            else
            {
                it++;
            }
        }
    }
}
