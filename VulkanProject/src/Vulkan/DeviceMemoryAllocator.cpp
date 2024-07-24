#include "DeviceMemoryAllocator.h"
#include "Helpers.h"
#include "MathHelper.h"
#include "Device.h"
#include <assert.h>

//#define ALLOCATOR_DEBUG
#define MB(bytes) bytes * 1024 * 1024

constexpr float mb = 1024.0f * 1024.0f;

FDeviceMemoryPage::FDeviceMemoryPage(VkDevice Device, VkPhysicalDevice PhyicalDevice, uint32_t Id, VkDeviceSize SizeInBytes, uint32_t MemoryType, VkMemoryPropertyFlags Properties)
    : m_Device(Device)
    , m_PhysicalDevice(PhyicalDevice)
    , m_Properties(Properties)
    , m_ID(Id)
    , m_MemoryType(MemoryType)
    , m_SizeInBytes(SizeInBytes)
    , m_BlockCount(0)
    , m_pHead(nullptr)
    , m_pHostMemory(nullptr)
    , m_IsMapped(false)
{
    Init();
}

FDeviceMemoryPage::~FDeviceMemoryPage()
{
    if (m_DeviceMemory != VK_NULL_HANDLE)
    {
        // Unmap
        Unmap();

        // Print MemoryLeaks
    #if defined(ALLOCATOR_DEBUG)
        {
            FDeviceMemoryBlock* pDebug = m_pHead;
            LOG("Allocated blocks left in MemoryPage '" << m_ID << "'\n");
            while (pDebug)
            {
                LOG("    Block: ID=" << pDebug->ID << ", Offset=" << pDebug->DeviceMemoryOffset << ", Size=" << pDebug->SizeInBytes << ", IsFree=" << std::boolalpha << pDebug->IsFree << std::endl;
                pDebug = pDebug->pNext;
            }
        }
    #endif

        // Delete all blocks
        FDeviceMemoryBlock* pCurrent = m_pHead;
        while (pCurrent != nullptr)
        {
            FDeviceMemoryBlock* pOld = pCurrent;
            pCurrent = pCurrent->pNext;

            //Delete block
            delete pOld;
        }

        // Free memory
        vkFreeMemory(m_Device, m_DeviceMemory, nullptr);
        m_DeviceMemory = VK_NULL_HANDLE;

        LOG("Deallocated MemoryPage\n");
    }
}

void FDeviceMemoryPage::Init()
{
    //Allocate device memory
    VkMemoryAllocateInfo AllocateInfo = {};
    AllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.pNext           = nullptr;
    AllocateInfo.allocationSize  = m_SizeInBytes;
    AllocateInfo.memoryTypeIndex = m_MemoryType;

    if (vkAllocateMemory(m_Device, &AllocateInfo, nullptr, &m_DeviceMemory) != VK_SUCCESS)
    {
        LOG("vkAllocateMemory failed\n");
        return;
    }
    else
    {
        LOG("Allocated '%llu' bytes for MemoryPage\n", m_SizeInBytes);
    }

    // Setup first block
    m_pHead = new FDeviceMemoryBlock();
    m_pHead->pPage              = this;
    m_pHead->pNext              = nullptr;
    m_pHead->pPrevious          = nullptr;
    m_pHead->IsFree             = true;
    m_pHead->ID                 = m_BlockCount++;
    m_pHead->SizeInBytes        = m_SizeInBytes;
    m_pHead->PaddedSizeInBytes  = m_SizeInBytes;
    m_pHead->DeviceMemoryOffset = 0;

    // If this is CPU visible -> Map
    if (m_Properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        Map();
    }
}

bool FDeviceMemoryPage::Allocate(FDeviceAllocation& Allocation, VkDeviceSize SizeInBytes, VkDeviceSize Alignment, VkDeviceSize Granularity)
{
    VkDeviceSize PaddedDeviceOffset = 0;
    VkDeviceSize PaddedSizeInBytes  = 0;
    FDeviceMemoryBlock* pBestFit = nullptr;

    // Find enough free space, and find the block that best fits
    for (FDeviceMemoryBlock* pCurrent = m_pHead; pCurrent != nullptr; pCurrent = pCurrent->pNext)
    {
        // Check if the block is allocated or not
        if (!pCurrent->IsFree)
        {
            continue;
        }

        // Does it fit into the block
        if (SizeInBytes > pCurrent->SizeInBytes)
        {
            continue;
        }

        // Align the offset
        PaddedDeviceOffset = Math::AlignUp<uint64_t>(pCurrent->DeviceMemoryOffset, Alignment);

        // Take granularity into account
        if (pCurrent->pPrevious != nullptr && Granularity > 1)
        {
            FDeviceMemoryBlock* pPrevious = pCurrent->pPrevious;
            if (IsOnSamePage(pPrevious->DeviceMemoryOffset, pPrevious->SizeInBytes, PaddedDeviceOffset, Granularity))
            {
                PaddedDeviceOffset = Math::AlignUp(PaddedDeviceOffset, Granularity);
            }
        }

        // Calculate padding
        PaddedSizeInBytes = SizeInBytes + (PaddedDeviceOffset - pCurrent->DeviceMemoryOffset);

        // Does it still fit
        if (PaddedSizeInBytes > pCurrent->SizeInBytes)
        {
            continue;
        }

        // Avoid granularity conflict
        if (Granularity > 1 && pCurrent->pNext != nullptr)
        {
            FDeviceMemoryBlock* pNext = pCurrent->pNext;
            if (IsOnSamePage(PaddedDeviceOffset, SizeInBytes, pNext->DeviceMemoryOffset, Granularity))
            {
                continue;
            }
        }

        pBestFit = pCurrent;
        break;
    }

    // Did we find a suitable block to make the allocation?
    if (pBestFit == nullptr)
    {
        return false;
    }

    //         Free block
    // |--------------------------|
    // padding Allocation Remaining
    // |------|----------|--------|
    if (pBestFit->SizeInBytes > PaddedSizeInBytes)
    {
        // Create a new block after allocation
        FDeviceMemoryBlock* pBlock = new FDeviceMemoryBlock();
        pBlock->pPage              = this;
        pBlock->ID                 = m_BlockCount++;
        pBlock->SizeInBytes        = pBestFit->SizeInBytes - PaddedSizeInBytes;
        pBlock->PaddedSizeInBytes  = pBlock->SizeInBytes;
        pBlock->DeviceMemoryOffset = pBestFit->DeviceMemoryOffset + PaddedSizeInBytes;
        pBlock->IsFree = true;

        //Set pointers
        pBlock->pNext     = pBestFit->pNext;
        pBlock->pPrevious = pBestFit;
        if (pBestFit->pNext)
        {
            pBestFit->pNext->pPrevious = pBlock;
        }

        pBestFit->pNext = pBlock;
    }

    //Update bestfit
    pBestFit->SizeInBytes       = SizeInBytes;
    pBestFit->PaddedSizeInBytes = PaddedSizeInBytes;
    pBestFit->IsFree = false;

    //Setup allocation
    Allocation.pBlock             = pBestFit;
    Allocation.DeviceMemory       = m_DeviceMemory;
    Allocation.DeviceMemoryOffset = PaddedDeviceOffset;
    Allocation.SizeInBytes        = SizeInBytes;
    if (m_Properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        Allocation.pHostMemory = m_pHostMemory + Allocation.DeviceMemoryOffset;
    }
    else
    {
        Allocation.pHostMemory = nullptr;
    }

#if defined (ALLOCATOR_DEBUG)
    {
        LOG("Memory Page '" << m_ID << "'\n");
        for (FDeviceMemoryBlock* pCurrent = m_pHead; pCurrent != nullptr; pCurrent = pCurrent->pNext)
        {
            LOG("----Block " << pCurrent->ID << "----\n");
            LOG("Starts at: Dec=" << pCurrent->DeviceMemoryOffset << ", Hex=" << std::hex << pCurrent->DeviceMemoryOffset << std::dec << std::endl;
            LOG("Free: " << std::boolalpha << pCurrent->IsFree << std::endl;
            
            if (pCurrent->pPrevious)
            {
                FDeviceMemoryBlock* pPrevious = pCurrent->pPrevious;
                if ((pPrevious->DeviceMemoryOffset + pPrevious->PaddedSizeInBytes) > pCurrent->DeviceMemoryOffset)
                {
                    LOG("Overlapping memory in page '" << m_ID << "' between blocks '" << pPrevious->ID << "' and '" << pCurrent->ID << std::endl;
                }
            }

            LOG("Padding=" << pCurrent->PaddedSizeInBytes - pCurrent->SizeInBytes << std::endl;
            LOG("SizeInBytes=" << pCurrent->PaddedSizeInBytes << std::endl;

            VkDeviceSize end = pCurrent->DeviceMemoryOffset + pCurrent->PaddedSizeInBytes;
            LOG("End at: Dec=" << end << ", Hex=" << std::hex << end << std::dec << std::endl;
            LOG("----------------\n");
        }
    }
#endif

    return true;
}

bool FDeviceMemoryPage::IsOnSamePage(VkDeviceSize OffsetA, VkDeviceSize SizeA, VkDeviceSize OffsetB, VkDeviceSize PageSize)
{
    assert(OffsetA + SizeA <= OffsetB && SizeA > 0 && PageSize > 0);

    VkDeviceSize aEnd       = OffsetA + (SizeA - 1);
    VkDeviceSize aEndPage   = aEnd & ~(PageSize - 1);
    VkDeviceSize bStart     = OffsetB;
    VkDeviceSize bStartPage = bStart & ~(PageSize - 1);
    return aEndPage == bStartPage;
}

void FDeviceMemoryPage::Map()
{
    // If not mapped -> map
    if (!m_IsMapped)
    {
        void* pMemory = nullptr;
        vkMapMemory(m_Device, m_DeviceMemory, 0, VK_WHOLE_SIZE, 0, &pMemory);

        m_pHostMemory = reinterpret_cast<uint8_t*>(pMemory);
        m_IsMapped    = true;
    }
}

void FDeviceMemoryPage::Unmap()
{
    // If mapped -> unmap
    if (m_IsMapped)
    {
        vkUnmapMemory(m_Device, m_DeviceMemory);
        m_pHostMemory = nullptr;
        m_IsMapped    = false;
    }
}

void FDeviceMemoryPage::Deallocate(FDeviceAllocation& Allocation)
{

    // Try to find the correct block
    FDeviceMemoryBlock* pCurrent = Allocation.pBlock;
    if (!pCurrent)
    {
        LOG("Block owning allocation was not found\n");
        return;
    }

    LOG("Deallocating Block ID=%u\n", pCurrent->ID);

    // Set this block to free
    pCurrent->IsFree = true;

    // Merge previous with current
    if (pCurrent->pPrevious)
    {
        FDeviceMemoryBlock* pPrevious = pCurrent->pPrevious;
        if (pPrevious->IsFree)
        {
            // Set size
            pPrevious->SizeInBytes       += pCurrent->PaddedSizeInBytes;
            pPrevious->PaddedSizeInBytes += pCurrent->PaddedSizeInBytes;

            // Set pointers
            pPrevious->pNext = pCurrent->pNext;
            if (pCurrent->pNext)
            {
                pCurrent->pNext->pPrevious = pPrevious;
            }

            // Remove block
            delete pCurrent;
            pCurrent = pPrevious;
        }
    }

    // Try and merge current with next
    if (pCurrent->pNext)
    {
        FDeviceMemoryBlock* pNext = pCurrent->pNext;
        if (pNext->IsFree)
        {
            // Set size
            pCurrent->SizeInBytes       += pNext->PaddedSizeInBytes;
            pCurrent->PaddedSizeInBytes += pNext->PaddedSizeInBytes;

            // Set pointers
            if (pNext->pNext)
            {
                pNext->pNext->pPrevious = pCurrent;
            }

            pCurrent->pNext = pNext->pNext;

            // Remove block
            delete pNext;
        }
    }
}

constexpr size_t NumFrames = 3;

FDeviceMemoryAllocator::FDeviceMemoryAllocator(FDevice* pDevice)
    : FDeviceChild(pDevice)
    , m_MaxAllocations(0)
    , m_TotalReserved(0)
    , m_TotalAllocated(0)
    , m_FrameIndex(0)
    , m_Pages()
    , m_GarbageMemory()
{
    // Resize the number of garbage memory vectors
    m_GarbageMemory.resize(NumFrames);

    // Setup from properties of the device
    VkPhysicalDeviceProperties Properties = {};
    vkGetPhysicalDeviceProperties(pDevice->GetPhysicalDevice(), &Properties);

    m_MaxAllocations         = Properties.limits.maxMemoryAllocationCount;
    m_BufferImageGranularity = Properties.limits.bufferImageGranularity;
}

FDeviceMemoryAllocator::~FDeviceMemoryAllocator()
{
    // Cleanup all garbage memory before deleting
    for (uint32_t i = 0; i < NumFrames; i++)
    {
        EmptyGarbageMemory();
    }

    // Delete allocator
    LOG("Deleting DeviceAllocator. Number of Pages: %u\n", m_Pages.size());

    for (FDeviceMemoryPage* page : m_Pages)
    {
        delete page;
    }

    LOG("Destroyed DeviceAllocator\n");
}

bool FDeviceMemoryAllocator::Allocate(FDeviceAllocation& Allocation, const VkMemoryRequirements& MemoryRequirements, VkMemoryPropertyFlags Properties)
{
    m_TotalAllocated += MemoryRequirements.size;
    uint32_t MemoryType = FindMemoryType(GetDevice()->GetPhysicalDevice(), MemoryRequirements.memoryTypeBits, Properties);

    // Try allocating from existing page
    for (auto Page : m_Pages)
    {
        if (Page->GetMemoryType() == MemoryType)
        {
            if (Page->Allocate(Allocation, MemoryRequirements.size, MemoryRequirements.alignment, m_BufferImageGranularity))
            {
                LOG("Allocated '%llu' bytes. Memory-Type=%u, Total Allocated: %.4f MB. Total Reserved: %.4f MB\n", MemoryRequirements.size, MemoryType, float(m_TotalAllocated) / mb, float(m_TotalReserved) / mb);
                return true;
            }
        }
    }

    assert(m_Pages.size() < m_MaxAllocations);

    // If allocated is large, make a dedicated allocation
    uint64_t BytesToReserve = MB(128);
    if (MemoryRequirements.size > BytesToReserve)
    {
        BytesToReserve = MemoryRequirements.size;
    }

    // Add to total
    m_TotalReserved += BytesToReserve;

    // Allocate new page
    FDeviceMemoryPage* pPage = new FDeviceMemoryPage(GetDevice()->GetDevice(), GetDevice()->GetPhysicalDevice(), uint32_t(m_Pages.size()), BytesToReserve, MemoryType, Properties);
    m_Pages.emplace_back(pPage);

    LOG("Allocated Memory-Page. Allocationcount: '%llu/%llu'. Memory-Type=%u, Total Allocated: %.4f MB. Total Reserved: %.4f MB\n", m_Pages.size(), m_MaxAllocations, MemoryType, float(m_TotalAllocated) / mb, float(m_TotalReserved) / mb);
    return pPage->Allocate(Allocation, MemoryRequirements.size, MemoryRequirements.alignment, m_BufferImageGranularity);
}

void FDeviceMemoryAllocator::Deallocate(FDeviceAllocation& Allocation)
{
    //Set it to be removed
    if (Allocation.pBlock && Allocation.DeviceMemory != VK_NULL_HANDLE)
    {
        m_GarbageMemory[m_FrameIndex].emplace_back(Allocation);
    }

    // Invalidate memory
    Allocation.pBlock             = nullptr;
    Allocation.DeviceMemoryOffset = 0;
    Allocation.SizeInBytes        = 0;
    Allocation.DeviceMemory       = VK_NULL_HANDLE;
    Allocation.pHostMemory        = nullptr;
}

void FDeviceMemoryAllocator::EmptyGarbageMemory()
{
    //Move on a frame
    m_FrameIndex = (m_FrameIndex + 1) % NumFrames;

    //Clean memory
    auto& MemoryBlocks = m_GarbageMemory[m_FrameIndex];
    if (!MemoryBlocks.empty())
    {
        //Deallocate all the blocks
        for (auto& Memory : MemoryBlocks)
        {
            if (Memory.pBlock && Memory.DeviceMemory != VK_NULL_HANDLE)
            {
                FDeviceMemoryPage* pPage = Memory.pBlock->pPage;
                pPage->Deallocate(Memory);

                m_TotalAllocated -= Memory.SizeInBytes;
                LOG("Allocated '%llu' bytes. Total Allocated: %.4f MB. Total Reserved: %.4f MB\n", Memory.SizeInBytes, float(m_TotalAllocated) / mb, float(m_TotalReserved) / mb);
            }
        }

        MemoryBlocks.clear();
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

                LOG("Destroyed Memory-Page. Allocationcount: '%llu/%llu'. Total Allocated: %.4f MB. Total Reserved: %.4f MB\n", m_Pages.size(), m_MaxAllocations, float(m_TotalAllocated) / mb, float(m_TotalReserved) / mb);
            }
            else
            {
                it++;
            }
        }
    }
}
