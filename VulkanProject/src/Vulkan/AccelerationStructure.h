#pragma once
#include "DeviceChild.h"

class FBuffer;
class FAccelerationStructure;

struct FBLASGeometry
{
    VkTransformMatrixKHR TransformMatrix;

    FBuffer* pVertexBuffer      = nullptr;
    uint32_t VertexBufferOffset = 0;
    uint32_t VertexBufferCount  = 0;
    uint32_t VertexStride       = 0;
    uint32_t MaxVertexIndex     = 0;

    FBuffer* pIndexBuffer       = nullptr;
    uint32_t IndexBufferOffset  = 0;
    uint32_t IndexBufferCount   = 0;
};

struct FAccelerationStructureBLASParams
{
    std::vector<FBLASGeometry> Geometries;
};

struct FAccelerationStructureTLASParams
{
    FAccelerationStructure* pAccelerationStructure = nullptr;
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