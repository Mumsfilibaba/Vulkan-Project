#include "Scene.h"
#include "Model.h"
#include "Application.h"

#define SPONZA 1

FScene::FScene(FDevice* pDevice)
    : m_pDevice(pDevice)
    , m_Camera()
    , m_CameraSpeed(1.0f)
    , m_pVertexBuffer(nullptr)
    , m_pIndexBuffer(nullptr)
    , m_pTopLevelAS(nullptr)
    , m_pBottomLevelAS(nullptr)
{
    assert(pDevice != nullptr);
}

FScene::~FScene()
{
    if (FDevice* pDevice = FApplication::Get().GetDevice())
    {
        pDevice->WaitForIdle();
    }

    SAFE_DELETE(m_pVertexBuffer);
    SAFE_DELETE(m_pIndexBuffer);
    SAFE_DELETE(m_pBottomLevelAS);
    SAFE_DELETE(m_pTopLevelAS);
}

void FScene::Initialize()
{
    FModel Model;

#if SPONZA
    Model.LoadFromFile(RESOURCE_PATH"/models/sponza/sponza.obj", FApplication::Get().GetDevice());
    m_CameraSpeed = 150.0f;
#else
    Model.LoadFromFile(RESOURCE_PATH"/models/queen.obj", FApplication::Get().GetDevice());
    m_CameraSpeed = 1.5f;
#endif

    FBufferParams VertexBufferParams = {};
    VertexBufferParams.Size             = Model.GetVertexCount() * sizeof(FVertex);
    VertexBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_RAY_TRACING_INPUT;
    VertexBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;

    m_pVertexBuffer = FBuffer::CreateAndCopy(FApplication::Get().GetDevice(), VertexBufferParams, nullptr, Model.GetVertexBuffer());
    assert(m_pVertexBuffer != nullptr);
    m_pVertexBuffer->SetDebugName("Scene VertexBuffer");

    FBufferParams IndexBufferParams = {};
    IndexBufferParams.Size             = Model.GetIndexCount() * sizeof(uint32_t);
    IndexBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_RAY_TRACING_INPUT;
    IndexBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;

    m_pIndexBuffer = FBuffer::CreateAndCopy(FApplication::Get().GetDevice(), IndexBufferParams, nullptr, Model.GetIndexBuffer());
    assert(m_pIndexBuffer != nullptr);
    m_pIndexBuffer->SetDebugName("Scene IndexBuffer");

    FMeshInfo& MeshInfo = m_MeshInfoBuffer.emplace_back();
    MeshInfo.VertexBufferAddress = m_pVertexBuffer->GetDeviceAddress().deviceAddress;
    MeshInfo.IndexBufferAddress  = m_pIndexBuffer->GetDeviceAddress().deviceAddress;

    FAccelerationStructureBLASParams BLASParams;
    BLASParams.pVertexBuffer = m_pVertexBuffer;
    BLASParams.pIndexBuffer  = m_pIndexBuffer;
    BLASParams.VertexCount   = Model.GetIndexCount();
    BLASParams.VertexStride  = sizeof(FVertex);

    m_pBottomLevelAS = FAccelerationStructure::CreateBLAS(FApplication::Get().GetDevice(), BLASParams);
    assert(m_pBottomLevelAS != nullptr);

    FAccelerationStructureTLASParams TLASParams;
    TLASParams.pAccelerationStructures = m_pBottomLevelAS;

    m_pTopLevelAS = FAccelerationStructure::CreateTLAS(FApplication::Get().GetDevice(), TLASParams);
    assert(m_pTopLevelAS != nullptr);
}