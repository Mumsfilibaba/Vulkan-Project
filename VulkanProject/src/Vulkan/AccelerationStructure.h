#pragma once
#include "DeviceChild.h"

class CBuffer;
class CAccelerationStructure;

struct SBLASGeometry
{
    VkTransformMatrixKHR TransformMatrix;

    CBuffer* pVertexBuffer      = nullptr;
    uint32_t VertexBufferOffset = 0;
    uint32_t VertexBufferCount  = 0;
    uint32_t VertexStride       = 0;
    uint32_t MaxVertexIndex     = 0;

    CBuffer* pIndexBuffer       = nullptr;
    uint32_t IndexBufferOffset  = 0;
    uint32_t IndexBufferCount   = 0;

    VkGeometryFlagsKHR Flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
};

struct SAccelerationStructureBLASParams
{
    std::vector<SBLASGeometry> Geometries;
};

struct STLASInstance
{
    VkTransformMatrixKHR    TransformMatrix     = { };
    CAccelerationStructure* pBLAS               = nullptr;
    uint32_t                InstanceCustomIndex = 0;
    bool                    bDisableCulling      = false;
    bool                    bFlipTriangleFacing = false; 
};

struct SAccelerationStructureTLASParams
{
    std::vector<STLASInstance> Instances;
};

class CAccelerationStructure : public CDeviceChild
{
public:
    static CAccelerationStructure* CreateBLAS(CDevice* pDevice, const SAccelerationStructureBLASParams& Params);
    static CAccelerationStructure* CreateTLAS(CDevice* pDevice, const SAccelerationStructureTLASParams& Params);

    CAccelerationStructure(CDevice* pDevice);
    ~CAccelerationStructure();

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