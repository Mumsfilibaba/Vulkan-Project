#include "Buffer.h"
#include "Helpers.h"
#include "Device.h"
#include "CommandBuffer.h"

CBuffer* CBuffer::Create(CDevice* pDevice, const SBufferParams& Params, CDeviceMemoryAllocator* pAllocator)
{
    CBuffer* pBuffer = new CBuffer(pDevice, pAllocator);
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
        LOG("vkCreateBuffer failed. Error: %d\n", Result);
        SAFE_DELETE(pBuffer);
        return nullptr;
    }
    else
    {
        LOG("Created Buffer\n");
    }

    VkMemoryRequirements MemoryRequirements = {};
    vkGetBufferMemoryRequirements(pDevice->GetDevice(), pBuffer->m_Buffer, &MemoryRequirements);
    
    /*if (pAllocator)
    {
        if (pAllocator->Allocate(pBuffer->m_Allocation, MemoryRequirements, Params.MemoryProperties))
        {
            vkBindBufferMemory(pDevice->GetDevice(), pBuffer->m_Buffer, pBuffer->m_Allocation.DeviceMemory, pBuffer->m_Allocation.DeviceMemoryOffset);
            pBuffer->m_Size          = Params.Size;
            pBuffer->m_AllocatedSize = MemoryRequirements.size;
        }
        else
        {
            LOG("VulkanDeviceAllocator::Allocate failed\n");
        }
    }
    else*/
    {
        VkMemoryAllocateInfo AllocInfo;
        ZERO_STRUCT(&AllocInfo);

        AllocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        AllocInfo.allocationSize  = MemoryRequirements.size;
        AllocInfo.memoryTypeIndex = FindMemoryType(pDevice->GetPhysicalDevice(), MemoryRequirements.memoryTypeBits, Params.MemoryProperties);

        VkMemoryAllocateFlagsInfo MemoryAllocateFlagsInfo = { };
        if (Params.Usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
        {
            ZERO_STRUCT(&MemoryAllocateFlagsInfo);
            MemoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
            MemoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
            AllocInfo.pNext = &MemoryAllocateFlagsInfo;
        }

        Result = vkAllocateMemory(pDevice->GetDevice(), &AllocInfo, nullptr, &pBuffer->m_DeviceMemory);
        if (Result != VK_SUCCESS)
        {
            LOG("vkAllocateMemory failed. Error: %d\n", Result);
        }
        else
        {
            vkBindBufferMemory(pDevice->GetDevice(), pBuffer->m_Buffer, pBuffer->m_DeviceMemory, 0);
            pBuffer->m_Size          = Params.Size;
            pBuffer->m_AllocatedSize = MemoryRequirements.size;

            LOG("Allocated %llu bytes\n", MemoryRequirements.size);
        }

        if (Params.Usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
        {
            VkBufferDeviceAddressInfoKHR BufferDeviceAddressInfo = { };
            BufferDeviceAddressInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            BufferDeviceAddressInfo.buffer = pBuffer->m_Buffer;

            VkDeviceAddress DeviceAddress = vkGetBufferDeviceAddress(pDevice->GetDevice(), &BufferDeviceAddressInfo);
            pBuffer->m_DeviceAddress.deviceAddress = DeviceAddress;
        }
    }
    
    return pBuffer;
}

CBuffer* CBuffer::CreateWithData(CDevice* pDevice, const SBufferParams& Params, CDeviceMemoryAllocator* pAllocator, const void* pSource)
{
    CBuffer* pBuffer = CBuffer::Create(pDevice, Params, pAllocator);
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
            SBufferParams BufferParams = {};
            BufferParams.Usage            = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            BufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;
            BufferParams.Size             = Params.Size;

            CBuffer* pUploadBuffer = CBuffer::CreateWithData(pDevice, BufferParams, nullptr, pSource);
            if (!pUploadBuffer)
            {
                SAFE_DELETE(pBuffer);
                return nullptr;
            }

            pUploadBuffer->SetDebugName("CBuffer::CreateWithData UploadBuffer");

            SCommandBufferParams CommandBufferParams = {};
            CommandBufferParams.Level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            CommandBufferParams.QueueType = ECommandQueueType::Graphics;

            CCommandBuffer* pCommandBuffer = CCommandBuffer::Create(pDevice, CommandBufferParams);
            if (!pCommandBuffer)
            {
                SAFE_DELETE(pUploadBuffer);
                SAFE_DELETE(pBuffer);
                return nullptr;
            }
            else
            {
                pCommandBuffer->SetDebugName("CBuffer::CreateWithData UploadCommandBuffer");
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

CBuffer* CBuffer::CreateAndCopy(CDevice* pDevice, const SBufferParams& Params, CDeviceMemoryAllocator* pAllocator, CBuffer* pSrcBuffer)
{
    if (!pSrcBuffer)
    {
        LOG("SrcBuffer cannot be nullptr");
        return nullptr;
    }

    CBuffer* pBuffer = CBuffer::Create(pDevice, Params, pAllocator);
    if (!pBuffer)
    {
        return nullptr;
    }

    SCommandBufferParams CommandBufferParams = {};
    CommandBufferParams.Level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferParams.QueueType = ECommandQueueType::Graphics;

    CCommandBuffer* pCommandBuffer = CCommandBuffer::Create(pDevice, CommandBufferParams);
    if (!pCommandBuffer)
    {
        SAFE_DELETE(pBuffer);
        return nullptr;
    }
    else
    {
        pCommandBuffer->SetDebugName("CBuffer::CreateAndCopy CommandBuffer");
    }

    pCommandBuffer->Reset();
    pCommandBuffer->Begin();

    VkBufferCopy BufferCopy;
    BufferCopy.size      = pSrcBuffer->GetSize();
    BufferCopy.dstOffset = 0;
    BufferCopy.srcOffset = 0;

    pCommandBuffer->CopyBuffer(pSrcBuffer->GetBuffer(), pBuffer->GetBuffer(), 1, &BufferCopy);
    pCommandBuffer->End();

    pDevice->ExecuteGraphics(pCommandBuffer, nullptr, nullptr);
    pDevice->WaitForIdle();

    SAFE_DELETE(pCommandBuffer);
    return pBuffer;
}

CBuffer::CBuffer(CDevice* pDevice, CDeviceMemoryAllocator* pAllocator)
    : CDeviceChild(pDevice)
    , m_pAllocator(pAllocator)
    , m_Buffer(VK_NULL_HANDLE)
    , m_DeviceMemory(VK_NULL_HANDLE)
    , m_Size(0)
    , m_AllocatedSize(0)
    , m_Allocation()
{
}

CBuffer::~CBuffer()
{
    if (m_Buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(GetDevice()->GetDevice(), m_Buffer, nullptr);
        m_Buffer = VK_NULL_HANDLE;
    }

    if (m_DeviceMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(GetDevice()->GetDevice(), m_DeviceMemory, nullptr);
        m_DeviceMemory = VK_NULL_HANDLE;
    }
    else
    {
        assert(m_pAllocator != nullptr);
        m_pAllocator->Deallocate(m_Allocation);
    }
}

void* CBuffer::Map()
{
    void* pResult = nullptr;
    if (m_pAllocator)
    {
        pResult = (void*)m_Allocation.pHostMemory;
    }
    else
    {
        VkResult Result = vkMapMemory(GetDevice()->GetDevice(), m_DeviceMemory, 0, m_AllocatedSize, 0, &pResult);
        if (Result != VK_SUCCESS)
        {
            LOG("vkMapMemory failed. Error: %d\n", Result);
            assert(false);
        }
    }
    
    return pResult;
}

void CBuffer::FlushMappedMemoryRange()
{
    if (!m_pAllocator)
    {
        VkMappedMemoryRange Range = {};
        ZERO_STRUCT(&Range);

        Range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        Range.memory = m_DeviceMemory;
        Range.offset = 0;
        Range.size   = m_AllocatedSize;

        VkResult Result = vkFlushMappedMemoryRanges(GetDevice()->GetDevice(), 1, &Range);
        if (Result != VK_SUCCESS)
        {
            LOG("vkFlushMappedMemoryRanges failed. Error: %d\n", Result);
            assert(false);
        }
    }
}

void CBuffer::Unmap()
{
    if (!m_pAllocator)
    {
        vkUnmapMemory(GetDevice()->GetDevice(), m_DeviceMemory);
    }
}

void CBuffer::SetDebugName(const char* DebugName)
{
    if (Extensions::vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT DebugNameInfo;
        ZERO_STRUCT(&DebugNameInfo);

        DebugNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        DebugNameInfo.objectType   = VK_OBJECT_TYPE_BUFFER;
        DebugNameInfo.pObjectName  = DebugName;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_Buffer);

        VkResult Result = Extensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            LOG("Failed to set name '%s'. Error: %d\n", DebugNameInfo.pObjectName, Result);
        }

        if (m_DeviceMemory != VK_NULL_HANDLE)
        {
            DebugNameInfo.objectType   = VK_OBJECT_TYPE_DEVICE_MEMORY;
            DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_DeviceMemory);

            Result = Extensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
            if (Result != VK_SUCCESS)
            {
                LOG("Failed to set name '%s'. Error: %d\n", DebugNameInfo.pObjectName, Result);
            }
        }
    }
}