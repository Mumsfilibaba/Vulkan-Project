#include "AccelerationStructure.h"
#include "Device.h"
#include "CommandBuffer.h"
#include "Buffer.h"
#include "Helpers.h"

struct SScratchBuffer : public CDeviceChild
{
    SScratchBuffer(CDevice* pDevice)
        : CDeviceChild(pDevice)
        , Buffer(VK_NULL_HANDLE)
        , DeviceMemory(VK_NULL_HANDLE)
        , DeviceAddress(0)
    {
    }

    ~SScratchBuffer()
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

static SScratchBuffer CreateScratchBuffer(CDevice* pDevice, VkDeviceSize size)
{
    SScratchBuffer ScratchBuffer(pDevice);

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

    VkBufferDeviceAddressInfoKHR BufferDeviceAddressInfo = { };
    BufferDeviceAddressInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    BufferDeviceAddressInfo.buffer = ScratchBuffer.Buffer;

    ScratchBuffer.DeviceAddress = vkGetBufferDeviceAddress(pDevice->GetDevice(), &BufferDeviceAddressInfo);
    return ScratchBuffer;
}

CAccelerationStructure* CAccelerationStructure::CreateBLAS(CDevice* pDevice, const SAccelerationStructureBLASParams& Params)
{
    if (Params.Geometries.empty())
    {
        LOG("No valid Geometries");
        return nullptr;
    }

    // Create a buffer for the transform-matrices
    std::vector<VkTransformMatrixKHR> TransformMatrices;
    for (const SBLASGeometry& Geometry : Params.Geometries)
    {
        TransformMatrices.emplace_back(Geometry.TransformMatrix);
    }

    SBufferParams TransformBufferParams = { };
    TransformBufferParams.Size             = TransformMatrices.size() * sizeof(VkTransformMatrixKHR);
    TransformBufferParams.Usage            = VK_BUFFER_USAGE_RAY_TRACING_INPUT;
    TransformBufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;

    CBuffer* pTransformBuffer = CBuffer::CreateWithData(pDevice, TransformBufferParams, nullptr, TransformMatrices.data());
    if (!pTransformBuffer)
    {
        LOG("Failed to create TransformBuffer\n");
        return nullptr;
    }
    else
    {
        pTransformBuffer->SetDebugName("BLAS TransformBuffer");
    }

    // Create AccelerationStructure
    CAccelerationStructure* pAccelerationStructure = new CAccelerationStructure(pDevice);

    // Gather all geometries
    std::vector<VkAccelerationStructureGeometryKHR>              AccelerationStructureGeometries;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR>        AccelerationStructureRangeInfos;
    std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> AccelerationStructureRangeInfoPointers;
    std::vector<uint32_t>                                        MaxPrimitiveCounts;

    for (const SBLASGeometry& Geometry : Params.Geometries)
    {
        VkAccelerationStructureGeometryKHR AccelerationStructureGeometry = { };
        AccelerationStructureGeometry.sType                           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        AccelerationStructureGeometry.flags                           = Geometry.Flags;
        AccelerationStructureGeometry.geometryType                    = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        AccelerationStructureGeometry.geometry.triangles.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        AccelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        AccelerationStructureGeometry.geometry.triangles.vertexData   = Geometry.pVertexBuffer->GetDeviceAddress();
        AccelerationStructureGeometry.geometry.triangles.maxVertex    = Geometry.MaxVertexIndex;
        AccelerationStructureGeometry.geometry.triangles.vertexStride = Geometry.VertexStride;

        AccelerationStructureGeometry.geometry.triangles.transformData                = pTransformBuffer->GetDeviceAddress();
        AccelerationStructureGeometry.geometry.triangles.transformData.deviceAddress += AccelerationStructureGeometries.size() * sizeof(VkTransformMatrixKHR);

        if (Geometry.pIndexBuffer)
        {
            AccelerationStructureGeometry.geometry.triangles.indexType                = VK_INDEX_TYPE_UINT32;
            AccelerationStructureGeometry.geometry.triangles.indexData                = Geometry.pIndexBuffer->GetDeviceAddress();
            AccelerationStructureGeometry.geometry.triangles.indexData.deviceAddress += Geometry.IndexBufferOffset * sizeof(uint32_t);
        }
        else
        {
            AccelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
            AccelerationStructureGeometry.geometry.triangles.indexData = VkDeviceOrHostAddressConstKHR{ 0 };
        }

        AccelerationStructureGeometries.push_back(AccelerationStructureGeometry);

        // Number of triangles from indexed or non-indexed geometry.
        const uint32_t NumTriangles = Geometry.pIndexBuffer ? (Geometry.IndexBufferCount / 3) : (Geometry.VertexBufferCount / 3);
        assert(Geometry.pIndexBuffer ? ((Geometry.IndexBufferCount % 3) == 0) : ((Geometry.VertexBufferCount % 3) == 0));
        MaxPrimitiveCounts.emplace_back(NumTriangles);

        VkAccelerationStructureBuildRangeInfoKHR AccelerationStructureBuildRangeInfo = { };
        AccelerationStructureBuildRangeInfo.primitiveCount  = NumTriangles;
        AccelerationStructureBuildRangeInfo.primitiveOffset = 0;
        AccelerationStructureBuildRangeInfo.firstVertex     = 0;
        AccelerationStructureBuildRangeInfo.transformOffset = 0;
        AccelerationStructureRangeInfos.push_back(AccelerationStructureBuildRangeInfo);
    }

    for (const VkAccelerationStructureBuildRangeInfoKHR& RangeInfo : AccelerationStructureRangeInfos)
    {
        AccelerationStructureRangeInfoPointers.emplace_back(&RangeInfo);
    }

    // Setup the build struct
    VkAccelerationStructureBuildGeometryInfoKHR AccelerationStructureBuildGeometryInfo = { };
    AccelerationStructureBuildGeometryInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    AccelerationStructureBuildGeometryInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    AccelerationStructureBuildGeometryInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    AccelerationStructureBuildGeometryInfo.pGeometries   = AccelerationStructureGeometries.data();
    AccelerationStructureBuildGeometryInfo.geometryCount = AccelerationStructureGeometries.size();

    // Retrieve the size of the AccelerationStructure
    VkAccelerationStructureBuildSizesInfoKHR AccelerationStructureBuildSizesInfo = { };
    AccelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    Extensions::vkGetAccelerationStructureBuildSizesKHR(pDevice->GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &AccelerationStructureBuildGeometryInfo, MaxPrimitiveCounts.data(), &AccelerationStructureBuildSizesInfo);

    // Create the AccelerationStructureBuffer
    VkBufferCreateInfo AccelerationStructureBufferCreateInfo = { };
    AccelerationStructureBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    AccelerationStructureBufferCreateInfo.size  = AccelerationStructureBuildSizesInfo.accelerationStructureSize;
    AccelerationStructureBufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    VkResult Result = vkCreateBuffer(pDevice->GetDevice(), &AccelerationStructureBufferCreateInfo, nullptr, &pAccelerationStructure->m_Buffer);
    if (Result != VK_SUCCESS)
    {
        LOG("AccelerationStructure vkCreateBuffer failed. Error: %d\n", Result);
        SAFE_DELETE(pTransformBuffer);
        SAFE_DELETE(pAccelerationStructure);
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

    VkMemoryAllocateInfo MemoryAllocateInfo = { };
    MemoryAllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    MemoryAllocateInfo.pNext           = &MemoryAllocateFlagsInfo;
    MemoryAllocateInfo.allocationSize  = MemoryRequirements.size;
    MemoryAllocateInfo.memoryTypeIndex = FindMemoryType(pDevice->GetPhysicalDevice(), MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Result = vkAllocateMemory(pDevice->GetDevice(), &MemoryAllocateInfo, nullptr, &pAccelerationStructure->m_DeviceMemory);
    if (Result != VK_SUCCESS)
    {
        LOG("AccelerationStructure vkAllocateMemory failed. Error: %d\n", Result);
        SAFE_DELETE(pTransformBuffer);
        SAFE_DELETE(pAccelerationStructure);
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
        SAFE_DELETE(pTransformBuffer);
        SAFE_DELETE(pAccelerationStructure);
        return nullptr;
    }
    else
    {
        LOG("Created AccelerationStructure\n");
    }

    // Create AccelerationStructure
    VkAccelerationStructureCreateInfoKHR AccelerationStructureCreateInfo = { };
    AccelerationStructureCreateInfo.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    AccelerationStructureCreateInfo.buffer = pAccelerationStructure->m_Buffer;
    AccelerationStructureCreateInfo.size   = AccelerationStructureBuildSizesInfo.accelerationStructureSize;
    AccelerationStructureCreateInfo.type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    Result = Extensions::vkCreateAccelerationStructureKHR(pDevice->GetDevice(), &AccelerationStructureCreateInfo, nullptr, &pAccelerationStructure->m_AccelerationStructure);
    if (Result != VK_SUCCESS)
    {
        LOG("vkCreateAccelerationStructureKHR failed. Error: %d\n", Result);
        SAFE_DELETE(pTransformBuffer);
        SAFE_DELETE(pAccelerationStructure);
        return nullptr;
    }
    else
    {
        LOG("Created AccelerationStructure\n");
    }

    // Create ScratchBuffer
    SScratchBuffer ScratchBuffer = CreateScratchBuffer(pDevice, AccelerationStructureBuildSizesInfo.buildScratchSize);
    if (!ScratchBuffer.IsValid())
    {
        SAFE_DELETE(pTransformBuffer);
        SAFE_DELETE(pAccelerationStructure);
        return nullptr;
    }

    // Prepare for build
    AccelerationStructureBuildGeometryInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    AccelerationStructureBuildGeometryInfo.dstAccelerationStructure  = pAccelerationStructure->m_AccelerationStructure;
    AccelerationStructureBuildGeometryInfo.scratchData.deviceAddress = ScratchBuffer.DeviceAddress;

    // Build AccelerationStructure
    SCommandBufferParams CommandBufferParams = { };
    CommandBufferParams.Level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferParams.QueueType = ECommandQueueType::Graphics;

    CCommandBuffer* pCommandBuffer = CCommandBuffer::Create(pDevice, CommandBufferParams);
    if (!pCommandBuffer)
    {
        SAFE_DELETE(pTransformBuffer);
        SAFE_DELETE(pAccelerationStructure);
        return nullptr;
    }
    else
    {
        pCommandBuffer->SetDebugName("BLAS Build CommandBuffer");
    }

    pCommandBuffer->Reset();
    pCommandBuffer->Begin();

    pCommandBuffer->BuildAccelerationStructures(1, &AccelerationStructureBuildGeometryInfo, AccelerationStructureRangeInfoPointers.data());

    pCommandBuffer->End();

    pDevice->ExecuteGraphics(pCommandBuffer, nullptr, nullptr);
    pDevice->WaitForIdle();

    // Get AccelerationStructure DeviceAddress
    VkAccelerationStructureDeviceAddressInfoKHR AccelerationDeviceAddressInfo = { };
    AccelerationDeviceAddressInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    AccelerationDeviceAddressInfo.accelerationStructure = pAccelerationStructure->m_AccelerationStructure;
    pAccelerationStructure->m_DeviceAddress = Extensions::vkGetAccelerationStructureDeviceAddressKHR(pDevice->GetDevice(), &AccelerationDeviceAddressInfo);

    SAFE_DELETE(pCommandBuffer);
    SAFE_DELETE(pTransformBuffer);
    return pAccelerationStructure;
}

CAccelerationStructure* CAccelerationStructure::CreateTLAS(CDevice* pDevice, const SAccelerationStructureTLASParams& Params)
{
    if (Params.Instances.empty())
    {
        LOG("No valid Instances");
        return nullptr;
    }

    CAccelerationStructure* pAccelerationStructure = new CAccelerationStructure(pDevice);

    // Create a buffer for the transform-matrices
    std::vector<VkAccelerationStructureInstanceKHR> Instances;
    for (const STLASInstance& InstanceParams : Params.Instances)
    {
        VkAccelerationStructureInstanceKHR Instance = { };
        Instance.flags = 0;

		if (InstanceParams.bFlipTriangleFacing)
		{
			Instance.flags |= VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR;
		}

        if (InstanceParams.bDisableCulling)
        {
            Instance.flags |= VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        }
        
        Instance.instanceCustomIndex                    = InstanceParams.InstanceCustomIndex;
        Instance.instanceShaderBindingTableRecordOffset = 0;
        Instance.mask                                   = 0xff;
        Instance.accelerationStructureReference         = InstanceParams.pBLAS->GetDeviceAddress();
        Instance.transform                              = InstanceParams.TransformMatrix;
        Instances.emplace_back(Instance);
    }

    // Buffer for instance data
    SBufferParams InstanceBufferParams = { };
    InstanceBufferParams.Size             = Instances.size() * sizeof(VkAccelerationStructureInstanceKHR);
    InstanceBufferParams.Usage            = VK_BUFFER_USAGE_RAY_TRACING_INPUT;
    InstanceBufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;

    CBuffer* pInstanceBuffer = CBuffer::CreateWithData(pDevice, InstanceBufferParams, nullptr, Instances.data());
    if (!pInstanceBuffer)
    {
        LOG("Failed to create InstanceBuffer\n");
        SAFE_DELETE(pAccelerationStructure);
        return nullptr;
    }
    else
    {
        pInstanceBuffer->SetDebugName("TLAS InstanceBuffer");
    }

    VkAccelerationStructureGeometryKHR AccelerationStructureGeometry = { };
    AccelerationStructureGeometry.sType                              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    AccelerationStructureGeometry.geometryType                       = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    AccelerationStructureGeometry.flags                              = VK_GEOMETRY_OPAQUE_BIT_KHR;
    AccelerationStructureGeometry.geometry.instances.sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    AccelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    AccelerationStructureGeometry.geometry.instances.data            = pInstanceBuffer->GetDeviceAddress();

    VkAccelerationStructureBuildGeometryInfoKHR AccelerationStructureBuildGeometryInfo = { };
    AccelerationStructureBuildGeometryInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    AccelerationStructureBuildGeometryInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    AccelerationStructureBuildGeometryInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    AccelerationStructureBuildGeometryInfo.geometryCount = 1;
    AccelerationStructureBuildGeometryInfo.pGeometries   = &AccelerationStructureGeometry;

    const uint32_t PrimitiveCount = Instances.size();
    VkAccelerationStructureBuildSizesInfoKHR AccelerationStructureBuildSizesInfo = { };
    AccelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    Extensions::vkGetAccelerationStructureBuildSizesKHR(pDevice->GetDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &AccelerationStructureBuildGeometryInfo, &PrimitiveCount, &AccelerationStructureBuildSizesInfo);

    VkBufferCreateInfo BufferCreateInfo = { };
    BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.size  = AccelerationStructureBuildSizesInfo.accelerationStructureSize;
    BufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    VkResult Result = vkCreateBuffer(pDevice->GetDevice(), &BufferCreateInfo, nullptr, &pAccelerationStructure->m_Buffer);
    if (Result != VK_SUCCESS)
    {
        LOG("AccelerationStructure vkCreateBuffer failed. Error: %d\n", Result);
        SAFE_DELETE(pAccelerationStructure);
        SAFE_DELETE(pInstanceBuffer);
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

    VkMemoryAllocateInfo MemoryAllocateInfo = { };
    MemoryAllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    MemoryAllocateInfo.pNext           = &MemoryAllocateFlagsInfo;
    MemoryAllocateInfo.allocationSize  = MemoryRequirements.size;
    MemoryAllocateInfo.memoryTypeIndex = FindMemoryType(pDevice->GetPhysicalDevice(), MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Result = vkAllocateMemory(pDevice->GetDevice(), &MemoryAllocateInfo, nullptr, &pAccelerationStructure->m_DeviceMemory);
    if (Result != VK_SUCCESS)
    {
        LOG("AccelerationStructure vkAllocateMemory failed. Error: %d\n", Result);
        SAFE_DELETE(pAccelerationStructure);
        SAFE_DELETE(pInstanceBuffer);
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
        SAFE_DELETE(pInstanceBuffer);
        return nullptr;
    }
    else
    {
        LOG("Created AccelerationStructure\n");
    }

    VkAccelerationStructureCreateInfoKHR AccelerationStructureCreateInfo = { };
    AccelerationStructureCreateInfo.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    AccelerationStructureCreateInfo.buffer = pAccelerationStructure->m_Buffer;
    AccelerationStructureCreateInfo.size   = AccelerationStructureBuildSizesInfo.accelerationStructureSize;
    AccelerationStructureCreateInfo.type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

    Result = Extensions::vkCreateAccelerationStructureKHR(pDevice->GetDevice(), &AccelerationStructureCreateInfo, nullptr, &pAccelerationStructure->m_AccelerationStructure);
    if (Result != VK_SUCCESS)
    {
        LOG("vkCreateAccelerationStructureKHR failed. Error: %d\n", Result);
        SAFE_DELETE(pAccelerationStructure);
        SAFE_DELETE(pInstanceBuffer);
        return nullptr;
    }
    else
    {
        LOG("Created AccelerationStructure\n");
    }

    SScratchBuffer ScratchBuffer = CreateScratchBuffer(pDevice, AccelerationStructureBuildSizesInfo.buildScratchSize);
    if (!ScratchBuffer.IsValid())
    {
        SAFE_DELETE(pAccelerationStructure);
        SAFE_DELETE(pInstanceBuffer);
        return nullptr;
    }

    VkAccelerationStructureBuildGeometryInfoKHR AccelerationBuildGeometryInfo = { };
    AccelerationBuildGeometryInfo.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    AccelerationBuildGeometryInfo.type                      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    AccelerationBuildGeometryInfo.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    AccelerationBuildGeometryInfo.mode                      = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    AccelerationBuildGeometryInfo.dstAccelerationStructure  = pAccelerationStructure->m_AccelerationStructure;
    AccelerationBuildGeometryInfo.geometryCount             = 1;
    AccelerationBuildGeometryInfo.pGeometries               = &AccelerationStructureGeometry;
    AccelerationBuildGeometryInfo.scratchData.deviceAddress = ScratchBuffer.DeviceAddress;

    VkAccelerationStructureBuildRangeInfoKHR AccelerationStructureBuildRangeInfo = { };
    AccelerationStructureBuildRangeInfo.primitiveCount  = Instances.size();
    AccelerationStructureBuildRangeInfo.primitiveOffset = 0;
    AccelerationStructureBuildRangeInfo.firstVertex     = 0;
    AccelerationStructureBuildRangeInfo.transformOffset = 0;

    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> AccelerationBuildStructureRangeInfos = { &AccelerationStructureBuildRangeInfo };

    SCommandBufferParams CommandBufferParams = { };
    CommandBufferParams.Level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferParams.QueueType = ECommandQueueType::Graphics;

    CCommandBuffer* pCommandBuffer = CCommandBuffer::Create(pDevice, CommandBufferParams);
    if (!pCommandBuffer)
    {
        SAFE_DELETE(pAccelerationStructure);
        SAFE_DELETE(pInstanceBuffer);
        return nullptr;
    }
    else
    {
        pCommandBuffer->SetDebugName("BLAS Build CommandBuffer");
    }

    pCommandBuffer->Reset();
    pCommandBuffer->Begin();

    pCommandBuffer->BuildAccelerationStructures(1, &AccelerationBuildGeometryInfo, AccelerationBuildStructureRangeInfos.data());

    pCommandBuffer->End();

    pDevice->ExecuteGraphics(pCommandBuffer, nullptr, nullptr);
    pDevice->WaitForIdle();

    VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
    accelerationDeviceAddressInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationDeviceAddressInfo.accelerationStructure = pAccelerationStructure->m_AccelerationStructure;
    pAccelerationStructure->m_DeviceAddress = Extensions::vkGetAccelerationStructureDeviceAddressKHR(pDevice->GetDevice(), &accelerationDeviceAddressInfo);

    SAFE_DELETE(pCommandBuffer);
    SAFE_DELETE(pInstanceBuffer);
    return pAccelerationStructure;
}

CAccelerationStructure::CAccelerationStructure(CDevice* pDevice)
    : CDeviceChild(pDevice)
    , m_DeviceAddress(0)
    , m_DeviceMemory(VK_NULL_HANDLE)
    , m_Buffer(VK_NULL_HANDLE)
    , m_AccelerationStructure(VK_NULL_HANDLE)
{
}

CAccelerationStructure::~CAccelerationStructure()
{
    if (m_AccelerationStructure != VK_NULL_HANDLE)
    {
        Extensions::vkDestroyAccelerationStructureKHR(GetDevice()->GetDevice(), m_AccelerationStructure, nullptr);
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

void CAccelerationStructure::SetDebugName(const char* DebugName)
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

        DebugNameInfo.objectType   = VK_OBJECT_TYPE_DEVICE_MEMORY;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_DeviceMemory);

        Result = Extensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            LOG("Failed to set name '%s'. Error: %d\n", DebugNameInfo.pObjectName, Result);
        }

        DebugNameInfo.objectType   = VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR ;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_AccelerationStructure);

        Result = Extensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            LOG("Failed to set name '%s'. Error: %d\n", DebugNameInfo.pObjectName, Result);
        }
    }
}