#include "AccelerationStructure.h"
#include "Device.h"
#include "CommandBuffer.h"
#include "Buffer.h"
#include "Helpers.h"

struct FScratchBuffer : public FDeviceChild
{
    FScratchBuffer(FDevice* pDevice)
        : FDeviceChild(pDevice)
        , Buffer(VK_NULL_HANDLE)
        , DeviceMemory(VK_NULL_HANDLE)
        , DeviceAddress(0)
    {
    }

    ~FScratchBuffer()
    {
        if (Buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(GetDevice()->GetDevice(), Buffer, nullptr);
            Buffer = VK_NULL_HANDLE;
        }

        if (DeviceMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(GetDevice()->GetDevice(), DeviceMemory, nullptr);
            DeviceMemory = VK_NULL_HANDLE;
        }

        DeviceAddress = 0;
    }

    bool IsValid() const
    {
        return Buffer != VK_NULL_HANDLE && DeviceMemory != VK_NULL_HANDLE && DeviceAddress != 0;
    }

    VkBuffer        Buffer;
    VkDeviceMemory  DeviceMemory;
    VkDeviceAddress DeviceAddress;
};

static FScratchBuffer CreateScratchBuffer(FDevice* pDevice, VkDeviceSize size)
{
    FScratchBuffer ScratchBuffer(pDevice);

    VkBufferCreateInfo BufferCreateInfo = { };
    BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.size  = size;
    BufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    VkResult Result = vkCreateBuffer(pDevice->GetDevice(), &BufferCreateInfo, nullptr, &ScratchBuffer.Buffer);
    if (Result != VK_SUCCESS)
    {
        LOG("Failed to create ScratchBuffer");
        return ScratchBuffer;
    }

    VkMemoryRequirements MemoryRequirements = { };
    vkGetBufferMemoryRequirements(pDevice->GetDevice(), ScratchBuffer.Buffer, &MemoryRequirements);

    VkMemoryAllocateFlagsInfo MemoryAllocateFlagsInfo = { };
    MemoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    MemoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

    VkMemoryAllocateInfo MemoryAllocateInfo = { };
    MemoryAllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    MemoryAllocateInfo.pNext           = &MemoryAllocateFlagsInfo;
    MemoryAllocateInfo.allocationSize  = MemoryRequirements.size;
    MemoryAllocateInfo.memoryTypeIndex = FindMemoryType(pDevice->GetPhysicalDevice(), MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Result = vkAllocateMemory(pDevice->GetDevice(), &MemoryAllocateInfo, nullptr, &ScratchBuffer.DeviceMemory);
    if (Result != VK_SUCCESS)
    {
        LOG("Failed to allocate ScratchBuffer memory");
        return ScratchBuffer;
    }

    Result = vkBindBufferMemory(pDevice->GetDevice(), ScratchBuffer.Buffer, ScratchBuffer.DeviceMemory, 0);
    if (Result != VK_SUCCESS)
    {
        LOG("Failed to bind ScratchBuffer memory");
        return ScratchBuffer;
    }

    VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo = { };
    bufferDeviceAddressInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAddressInfo.buffer = ScratchBuffer.Buffer;

    ScratchBuffer.DeviceAddress = vkGetBufferDeviceAddress(pDevice->GetDevice(), &bufferDeviceAddressInfo);
    return ScratchBuffer;
}

FAccelerationStructure* FAccelerationStructure::CreateBLAS(FDevice* pDevice, const FAccelerationStructureBLASParams& Params)
{
    if (!Params.pVertexBuffer)
    {
        LOG("No valid VertexBuffer");
        return nullptr;
    }

    FAccelerationStructure* pAccelerationStructure = new FAccelerationStructure(pDevice);

    VkAccelerationStructureGeometryKHR AccelerationStructureGeometry = { };
    AccelerationStructureGeometry.sType                           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    AccelerationStructureGeometry.flags                           = VK_GEOMETRY_OPAQUE_BIT_KHR;
    AccelerationStructureGeometry.geometryType                    = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    AccelerationStructureGeometry.geometry.triangles.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    AccelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    AccelerationStructureGeometry.geometry.triangles.vertexData   = Params.pVertexBuffer->GetDeviceAddress();
    AccelerationStructureGeometry.geometry.triangles.maxVertex    = Params.VertexCount;
    AccelerationStructureGeometry.geometry.triangles.vertexStride = Params.VertexStride;
    
    if (Params.pIndexBuffer)
    {
        AccelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
        AccelerationStructureGeometry.geometry.triangles.indexData = Params.pIndexBuffer->GetDeviceAddress();
    }
    else
    {
        AccelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
        AccelerationStructureGeometry.geometry.triangles.indexData = VkDeviceOrHostAddressConstKHR{ 0 };
    }

    // Use the specified TransformBuffer or create a new "Identity" TransformBuffer
    FBuffer* pTransformBuffer     = Params.pTransformBuffer;
    FBuffer* pTempTransformBuffer = nullptr;
    if (!pTransformBuffer)
    {
        VkTransformMatrixKHR TransformMatrix = 
        {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f
        };

        FBufferParams TransformBufferParams = { };
        TransformBufferParams.Size             = sizeof(VkTransformMatrixKHR);
        TransformBufferParams.Usage            = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_RAY_TRACING;
        TransformBufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;

        pTransformBuffer = pTempTransformBuffer = FBuffer::CreateWithData(pDevice, TransformBufferParams, nullptr, &TransformMatrix);
        if (!pTransformBuffer)
        {
            LOG("Failed to create TransformBuffer\n");
            SAFE_DELETE(pAccelerationStructure);
            return nullptr;
        }
    }

    assert(pTransformBuffer != nullptr);
    AccelerationStructureGeometry.geometry.triangles.transformData = pTransformBuffer->GetDeviceAddress();

    VkAccelerationStructureBuildGeometryInfoKHR AccelerationStructureBuildGeometryInfo = { };
    AccelerationStructureBuildGeometryInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    AccelerationStructureBuildGeometryInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    AccelerationStructureBuildGeometryInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    AccelerationStructureBuildGeometryInfo.geometryCount = 1;
    AccelerationStructureBuildGeometryInfo.pGeometries   = &AccelerationStructureGeometry;
    
    const uint32_t NumTriangles = 1;
    VkAccelerationStructureBuildSizesInfoKHR AccelerationStructureBuildSizesInfo = { };
    AccelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    FExtensions::vkGetAccelerationStructureBuildSizesKHR(pDevice->GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &AccelerationStructureBuildGeometryInfo, &NumTriangles, &AccelerationStructureBuildSizesInfo);

    VkBufferCreateInfo BufferCreateInfo = { };
    BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.size  = AccelerationStructureBuildSizesInfo.accelerationStructureSize;
    BufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    VkResult Result = vkCreateBuffer(pDevice->GetDevice(), &BufferCreateInfo, nullptr, &pAccelerationStructure->m_Buffer);
    if (Result != VK_SUCCESS)
    {
        LOG("AccelerationStructure vkCreateBuffer failed. Error: %d\n", Result);
        SAFE_DELETE(pAccelerationStructure);
        SAFE_DELETE(pTempTransformBuffer);
        return nullptr;
    }
    else
    {
        LOG("Created AccelerationStructure Buffer\n");
    }

    VkMemoryRequirements MemoryRequirements = { };
    vkGetBufferMemoryRequirements(pDevice->GetDevice(), pAccelerationStructure->m_Buffer, &MemoryRequirements);

    VkMemoryAllocateFlagsInfo MemoryAllocateFlagsInfo = { };
    MemoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    MemoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

    VkMemoryAllocateInfo MemoryAllocateInfo = {};
    MemoryAllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    MemoryAllocateInfo.pNext           = &MemoryAllocateFlagsInfo;
    MemoryAllocateInfo.allocationSize  = MemoryRequirements.size;
    MemoryAllocateInfo.memoryTypeIndex = FindMemoryType(pDevice->GetPhysicalDevice(), MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Result = vkAllocateMemory(pDevice->GetDevice(), &MemoryAllocateInfo, nullptr, &pAccelerationStructure->m_DeviceMemory);
    if (Result != VK_SUCCESS)
    {
        LOG("AccelerationStructure vkAllocateMemory failed. Error: %d\n", Result);
        SAFE_DELETE(pAccelerationStructure);
        SAFE_DELETE(pTempTransformBuffer);
        return nullptr;
    }
    else
    {
        LOG("Allocated AccelerationStructure Memory\n");
    }

    Result = vkBindBufferMemory(pDevice->GetDevice(), pAccelerationStructure->m_Buffer, pAccelerationStructure->m_DeviceMemory, 0);
    if (Result != VK_SUCCESS)
    {
        LOG("AccelerationStructure vkBindBufferMemory failed. Error: %d\n", Result);
        SAFE_DELETE(pAccelerationStructure);
        SAFE_DELETE(pTempTransformBuffer);
        return nullptr;
    }
    else
    {
        LOG("Created AccelerationStructure\n");
    }

    VkAccelerationStructureCreateInfoKHR AccelerationStructureCreateInfo = {};
    AccelerationStructureCreateInfo.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    AccelerationStructureCreateInfo.buffer = pAccelerationStructure->m_Buffer;
    AccelerationStructureCreateInfo.size   = AccelerationStructureBuildSizesInfo.accelerationStructureSize;
    AccelerationStructureCreateInfo.type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    Result = FExtensions::vkCreateAccelerationStructureKHR(pDevice->GetDevice(), &AccelerationStructureCreateInfo, nullptr, &pAccelerationStructure->m_AccelerationStructure);
    if (Result != VK_SUCCESS)
    {
        LOG("vkCreateAccelerationStructureKHR failed. Error: %d\n", Result);
        SAFE_DELETE(pAccelerationStructure);
        SAFE_DELETE(pTempTransformBuffer);
        return nullptr;
    }
    else
    {
        LOG("Created AccelerationStructure\n");
    }

    FScratchBuffer ScratchBuffer = CreateScratchBuffer(pDevice, AccelerationStructureBuildSizesInfo.buildScratchSize);
    if (!ScratchBuffer.IsValid())
    {
        SAFE_DELETE(pAccelerationStructure);
        return nullptr;
    }

    VkAccelerationStructureBuildGeometryInfoKHR AccelerationBuildGeometryInfo = { };
    AccelerationBuildGeometryInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    AccelerationBuildGeometryInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    AccelerationBuildGeometryInfo.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    AccelerationBuildGeometryInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    AccelerationBuildGeometryInfo.dstAccelerationStructure  = pAccelerationStructure->m_AccelerationStructure;
    AccelerationBuildGeometryInfo.geometryCount             = 1;
    AccelerationBuildGeometryInfo.pGeometries               = &AccelerationStructureGeometry;
    AccelerationBuildGeometryInfo.scratchData.deviceAddress = ScratchBuffer.DeviceAddress;

    VkAccelerationStructureBuildRangeInfoKHR AccelerationStructureBuildRangeInfo = { };
    AccelerationStructureBuildRangeInfo.primitiveCount  = NumTriangles;
    AccelerationStructureBuildRangeInfo.primitiveOffset = 0;
    AccelerationStructureBuildRangeInfo.firstVertex     = 0;
    AccelerationStructureBuildRangeInfo.transformOffset = 0;

    VkAccelerationStructureBuildRangeInfoKHR* AccelerationBuildStructureRangeInfos[] =
    { 
        &AccelerationStructureBuildRangeInfo
    };

    FCommandBufferParams CommandBufferParams = { };
    CommandBufferParams.Level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferParams.QueueType = ECommandQueueType::Graphics;

    FCommandBuffer* pCommandBuffer = FCommandBuffer::Create(pDevice, CommandBufferParams);
    if (!pCommandBuffer)
    {
        SAFE_DELETE(pAccelerationStructure);
        SAFE_DELETE(pTempTransformBuffer);
        return nullptr;
    }
    else
    {
        pCommandBuffer->SetDebugName("BLAS Build CommandBuffer");
    }

    pCommandBuffer->Reset();
    pCommandBuffer->Begin();

    pCommandBuffer->BuildAccelerationStructures(1, &AccelerationBuildGeometryInfo, AccelerationBuildStructureRangeInfos);

    pCommandBuffer->End();

    pDevice->ExecuteGraphics(pCommandBuffer, nullptr, nullptr);
    pDevice->WaitForIdle();

    VkAccelerationStructureDeviceAddressInfoKHR AccelerationDeviceAddressInfo = {};
    AccelerationDeviceAddressInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    AccelerationDeviceAddressInfo.accelerationStructure = pAccelerationStructure->m_AccelerationStructure;
    pAccelerationStructure->m_DeviceAddress = FExtensions::vkGetAccelerationStructureDeviceAddressKHR(pDevice->GetDevice(), &AccelerationDeviceAddressInfo);

    SAFE_DELETE(pCommandBuffer);
    SAFE_DELETE(pTempTransformBuffer);
    return pAccelerationStructure;
}

FAccelerationStructure* FAccelerationStructure::CreateTLAS(FDevice* pDevice, const FAccelerationStructureTLASParams& Params)
{
    return nullptr;
}

FAccelerationStructure::FAccelerationStructure(FDevice* pDevice)
    : FDeviceChild(pDevice)
{
}

FAccelerationStructure::~FAccelerationStructure()
{
    if (m_AccelerationStructure != VK_NULL_HANDLE)
    {
        FExtensions::vkDestroyAccelerationStructureKHR(GetDevice()->GetDevice(), m_AccelerationStructure, nullptr);
        m_AccelerationStructure = VK_NULL_HANDLE; 
    }

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
}

void FAccelerationStructure::SetDebugName(const char* DebugName)
{
    if (FExtensions::vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT DebugNameInfo;
        ZERO_STRUCT(&DebugNameInfo);

        DebugNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        DebugNameInfo.objectType   = VK_OBJECT_TYPE_BUFFER;
        DebugNameInfo.pObjectName  = DebugName;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_Buffer);

        VkResult Result = FExtensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            LOG("Failed to set name '%s'. Error: %d\n", DebugNameInfo.pObjectName, Result);
        }

        DebugNameInfo.objectType   = VK_OBJECT_TYPE_DEVICE_MEMORY;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_DeviceMemory);

        Result = FExtensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            LOG("Failed to set name '%s'. Error: %d\n", DebugNameInfo.pObjectName, Result);
        }

        DebugNameInfo.objectType   = VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR ;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_AccelerationStructure);

        Result = FExtensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            LOG("Failed to set name '%s'. Error: %d\n", DebugNameInfo.pObjectName, Result);
        }
    }
}