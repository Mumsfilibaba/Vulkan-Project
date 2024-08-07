#include "Scene.h"
#include "Model.h"
#include "Application.h"

#define SPONZA 1

FScene::FScene(FDevice* pDevice)
    : m_pDevice(pDevice)
    , m_Camera()
    , m_CameraSpeed(1.0f)
    , m_VertexBuffers()
    , m_IndexBuffers()
    , m_pTopLevelAS(nullptr)
    , m_BottomLevelASs()
    , m_pMaterialBuffer(nullptr)
    , m_pMaterialSampler(nullptr)
{
    assert(pDevice != nullptr);
}

FScene::~FScene()
{
    if (FDevice* pDevice = FApplication::Get().GetDevice())
    {
        pDevice->WaitForIdle();
    }

    for (FBuffer* pBuffer : m_VertexBuffers)
    {
        SAFE_DELETE(pBuffer);
    }
    for (FBuffer* pBuffer : m_IndexBuffers)
    {
        SAFE_DELETE(pBuffer);
    }
    for (FAccelerationStructure* pAccelerationStructure : m_BottomLevelASs)
    {
        SAFE_DELETE(pAccelerationStructure);
    }

    SAFE_DELETE(m_pTopLevelAS);
}

void FScene::Initialize()
{
    FModel* Model = new FModel();

#if SPONZA
    Model->LoadFromFile(RESOURCE_PATH"/models/sponza/sponza.obj", FApplication::Get().GetDevice());
    m_CameraSpeed = 150.0f;
#else
    Model->LoadFromFile(RESOURCE_PATH"/models/queen.obj", FApplication::Get().GetDevice());
    m_CameraSpeed = 1.5f;
#endif

    // Copy the buffers from the model
    FBufferParams VertexBufferParams = {};
    VertexBufferParams.Size             = Model->VertexCount * sizeof(FVertex);
    VertexBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_RAY_TRACING_INPUT;
    VertexBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;

    FBuffer* pVertexBuffer = FBuffer::CreateAndCopy(FApplication::Get().GetDevice(), VertexBufferParams, nullptr, Model->pVertexBuffer);
    assert(pVertexBuffer != nullptr);
    pVertexBuffer->SetDebugName("Scene VertexBuffer");

    FBufferParams IndexBufferParams = {};
    IndexBufferParams.Size             = Model->IndexCount * sizeof(uint32_t);
    IndexBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_RAY_TRACING_INPUT;
    IndexBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;

    FBuffer* pIndexBuffer = FBuffer::CreateAndCopy(FApplication::Get().GetDevice(), IndexBufferParams, nullptr, Model->pIndexBuffer);
    assert(pIndexBuffer != nullptr);
    pIndexBuffer->SetDebugName("Scene IndexBuffer");

    m_VertexBuffers.push_back(pVertexBuffer);
    m_IndexBuffers.push_back(pIndexBuffer);

    // Create geometries for the AccelerationStructure
    VkTransformMatrixKHR TransformMatrix =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f
    };

    FAccelerationStructureBLASParams BLASParams;
    for (const FModel::FSubMesh& SubMesh : Model->SubMeshes)
    {
        FBLASGeometry& Geometry = BLASParams.Geometries.emplace_back();
        Geometry.TransformMatrix    = TransformMatrix;
        Geometry.pVertexBuffer      = pVertexBuffer;
        Geometry.MaxVertexIndex     = Model->VertexCount;
        Geometry.VertexBufferCount  = SubMesh.VertexCount;
        Geometry.VertexBufferOffset = SubMesh.VertexOffset;
        Geometry.VertexStride       = sizeof(FVertex);
        Geometry.pIndexBuffer       = pIndexBuffer;
        Geometry.IndexBufferOffset  = SubMesh.IndexOffset;
        Geometry.IndexBufferCount   = SubMesh.IndexCount;

        FMeshInfo& MeshInfo = m_MeshInfoBuffer.emplace_back();
        MeshInfo.VertexBufferAddress = pVertexBuffer->GetDeviceAddress().deviceAddress;
        MeshInfo.IndexBufferAddress  = pIndexBuffer->GetDeviceAddress().deviceAddress;
        MeshInfo.IndexBufferAddress += Geometry.IndexBufferOffset * sizeof(uint32_t);
    }

    // Cleanup the old VertexBuffers
    SAFE_DELETE(Model);

    // Create Bottom-Level AccelerationStructure
    FAccelerationStructure* pBottomLevelAS = FAccelerationStructure::CreateBLAS(FApplication::Get().GetDevice(), BLASParams);
    assert(pBottomLevelAS != nullptr);
    m_BottomLevelASs.push_back(pBottomLevelAS);

    // Create Top-Level AccelerationStructure
    FAccelerationStructureTLASParams TLASParams;
    TLASParams.pAccelerationStructure = m_BottomLevelASs[0];

    m_pTopLevelAS = FAccelerationStructure::CreateTLAS(FApplication::Get().GetDevice(), TLASParams);
    assert(m_pTopLevelAS != nullptr);
}