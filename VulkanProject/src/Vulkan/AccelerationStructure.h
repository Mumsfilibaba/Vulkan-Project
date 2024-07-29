#pragma once
#include "DeviceChild.h"

class FBuffer;
class FAccelerationStructure;

struct FAccelerationStructureBLASParams
{
    FBuffer* pVertexBuffer    = nullptr;
    FBuffer* pIndexBuffer     = nullptr;
    FBuffer* pTransformBuffer = nullptr;
    uint32_t VertexCount      = 0;
    uint32_t VertexStride     = 0;
};

struct FAccelerationStructureTLASParams
{
    FAccelerationStructure* pAccelerationStructures = nullptr;
};

class FAccelerationStructure : public FDeviceChild
{
public:
    static FAccelerationStructure* CreateBLAS(FDevice* pDevice, const FAccelerationStructureBLASParams& Params);
    static FAccelerationStructure* CreateTLAS(FDevice* pDevice, const FAccelerationStructureTLASParams& Params);

    FAccelerationStructure(FDevice* pDevice);
    ~FAccelerationStructure();

    void SetDebugName(const char* DebugName);

    uint64_t GetDeviceAddress() const
    {
        return m_DeviceAddress;
    }

    VkAccelerationStructureKHR GetAccelerationStructure() const
    {
        return m_AccelerationStructure;
    }

private:
	uint64_t                   m_DeviceAddress;
	VkDeviceMemory             m_DeviceMemory;
	VkBuffer                   m_Buffer;
	VkAccelerationStructureKHR m_AccelerationStructure;
};