#include "Buffer.h"
#include "Helpers.h"
#include "Device.h"
#include "CommandBuffer.h"

FBuffer* FBuffer::Create(FDevice* pDevice, const FBufferParams& Params, FDeviceMemoryAllocator* pAllocator)
{
    FBuffer* pBuffer = new FBuffer(pDevice, pAllocator);
    assert(Params.Size > 0);

    VkBufferCreateInfo BufferCreateInfo;
    ZERO_STRUCT(&BufferCreateInfo);
    
    BufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.size        = Params.Size;
    BufferCreateInfo.usage       = Params.Usage;
    BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult Result = vkCreateBuffer(pDevice->GetDevice(), &BufferCreateInfo, nullptr, &pBuffer->m_Buffer);
    if (Result != VK_SUCCESS)
    {
        std::cout << "vkCreateBuffer failed. Error: " << Result << "\n";
        return nullptr;
    }
    else
    {
        std::cout << "Created Buffer\n";
    }

    VkMemoryRequirements MemoryRequirements = {};
    vkGetBufferMemoryRequirements(pDevice->GetDevice(), pBuffer->m_Buffer, &MemoryRequirements);
    
    if (pAllocator)
    {
        if (pAllocator->Allocate(pBuffer->m_Allocation, MemoryRequirements, Params.MemoryProperties))
        {
            vkBindBufferMemory(pDevice->GetDevice(), pBuffer->m_Buffer, pBuffer->m_Allocation.DeviceMemory, pBuffer->m_Allocation.DeviceMemoryOffset);
            pBuffer->m_Size = Params.Size;
        }
        else
        {
            std::cout << "VulkanDeviceAllocator::Allocate failed\n";
        }
    }
    else
    {
        VkMemoryAllocateInfo AllocInfo;
        ZERO_STRUCT(&AllocInfo);
        
        AllocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        AllocInfo.allocationSize  = MemoryRequirements.size;
        AllocInfo.memoryTypeIndex = FindMemoryType(pDevice->GetPhysicalDevice(), MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        Result = vkAllocateMemory(pDevice->GetDevice(), &AllocInfo, nullptr, &pBuffer->m_DeviceMemory);
        if (Result != VK_SUCCESS)
        {
            std::cout << "vkAllocateMemory failed. Error: " << Result << "\n";
        }
        else
        {
            vkBindBufferMemory(pDevice->GetDevice(), pBuffer->m_Buffer, pBuffer->m_DeviceMemory, 0);
            pBuffer->m_Size = MemoryRequirements.size;

            std::cout << "Allocated " << MemoryRequirements.size << " bytes\n";
        }
    }
    
    return pBuffer;
}

FBuffer* FBuffer::CreateWithData(FDevice* pDevice, const FBufferParams& Params, FDeviceMemoryAllocator* pAllocator, const void* pSource)
{
    FBuffer* pBuffer = FBuffer::Create(pDevice, Params, pAllocator);
    if (!pBuffer)
    {
        return nullptr;
    }
    
    if (pSource)
    {
        if (Params.MemoryProperties == VK_CPU_BUFFER_USAGE)
        {
            void* pData = pBuffer->Map();
            memcpy(pData, pSource, Params.Size);
            pBuffer->FlushMappedMemoryRange();
            pBuffer->Unmap();
        }
        else
        {
            FBufferParams BufferParams = {};
            BufferParams.Usage            = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            BufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;
            BufferParams.Size             = Params.Size;
            
            FBuffer* pUploadBuffer = FBuffer::CreateWithData(pDevice, BufferParams, nullptr, pSource);
            if (!pUploadBuffer)
            {
                SAFE_DELETE(pBuffer);
                return nullptr;
            }
            
            FCommandBufferParams CommandBufferParams = {};
            CommandBufferParams.Level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            CommandBufferParams.QueueType = ECommandQueueType::Graphics;
            
            FCommandBuffer* pCommandBuffer = FCommandBuffer::Create(pDevice, CommandBufferParams);
            if (!pCommandBuffer)
            {
                SAFE_DELETE(pUploadBuffer);
                SAFE_DELETE(pBuffer);
                return nullptr;
            }
            
            pCommandBuffer->Reset();
            pCommandBuffer->Begin();
            
            VkBufferCopy BufferCopy;
            BufferCopy.size      = pUploadBuffer->GetSize();
            BufferCopy.dstOffset = 0;
            BufferCopy.srcOffset = 0;
            
            pCommandBuffer->CopyBuffer(pUploadBuffer->GetBuffer(), pBuffer->GetBuffer(), 1, &BufferCopy);
            pCommandBuffer->End();
            
            pDevice->ExecuteGraphics(pCommandBuffer, nullptr, nullptr);
            pDevice->WaitForIdle();
            
            SAFE_DELETE(pUploadBuffer);
            SAFE_DELETE(pCommandBuffer);
        }
    }
    
    return pBuffer;
}

FBuffer::FBuffer(FDevice* pDevice, FDeviceMemoryAllocator* pAllocator)
    : FDeviceChild(pDevice)
    , m_pAllocator(pAllocator)
    , m_Buffer(VK_NULL_HANDLE)
    , m_DeviceMemory(VK_NULL_HANDLE)
    , m_Size(0)
    , m_Allocation()
{
}

FBuffer::~FBuffer()
{
    if (m_Buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(GetDevice()->GetDevice(), m_Buffer, nullptr);
        m_Buffer = VK_NULL_HANDLE;
    }

    if (m_pAllocator)
    {
        m_pAllocator->Deallocate(m_Allocation);
    }
    else
    {
        if (m_DeviceMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(GetDevice()->GetDevice(), m_DeviceMemory, nullptr);
            m_DeviceMemory = VK_NULL_HANDLE;
        }
    }
}

void* FBuffer::Map()
{
    void* pResult = nullptr;
    if (m_pAllocator)
    {
        pResult = (void*)m_Allocation.pHostMemory;
    }
    else
    {
        VkResult Result = vkMapMemory(GetDevice()->GetDevice(), m_DeviceMemory, 0, m_Size, 0, &pResult);
        if (Result != VK_SUCCESS)
        {
            std::cout << "vkMapMemory failed. Error: " << Result << "\n";
            assert(false);
        }
    }
    
    return pResult;
}

void FBuffer::FlushMappedMemoryRange()
{
    if (!m_pAllocator)
    {
        VkMappedMemoryRange Range = {};
        ZERO_STRUCT(&Range);

        Range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        Range.memory = m_DeviceMemory;
        Range.offset = 0;
        Range.size   = m_Size;

        VkResult Result = vkFlushMappedMemoryRanges(GetDevice()->GetDevice(), 1, &Range);
        if (Result != VK_SUCCESS)
        {
            std::cout << "vkFlushMappedMemoryRanges failed. Error: " << Result << "\n";
            assert(false);
        }
    }
}

void FBuffer::Unmap()
{
    if (!m_pAllocator)
    {
        vkUnmapMemory(GetDevice()->GetDevice(), m_DeviceMemory);
    }
}
