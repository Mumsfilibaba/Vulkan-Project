#include "Scene.h"
#include "Model.h"
#include "Application.h"

FScene::FScene()
    : m_Camera()
    , pTopLevelAS(nullptr)
    , pBottomLevelAS(nullptr)
{
}

FScene::~FScene()
{
    if (FDevice* pDevice = FApplication::Get().GetDevice())
    {
        pDevice->WaitForIdle();
    }

    SAFE_DELETE(pBottomLevelAS);
    SAFE_DELETE(pTopLevelAS);
}

void FScene::Initialize()
{
    FModel Model;
    Model.LoadFromFile(RESOURCE_PATH"/models/queen.obj", FApplication::Get().GetDevice());

    FAccelerationStructureBLASParams BLASParams;
    BLASParams.pVertexBuffer = Model.GetVertexBuffer();
    BLASParams.pIndexBuffer  = Model.GetIndexBuffer();
    BLASParams.VertexCount   = Model.GetIndexCount();
    BLASParams.VertexStride  = sizeof(FVertex);

    pBottomLevelAS = FAccelerationStructure::CreateBLAS(FApplication::Get().GetDevice(), BLASParams);
    assert(pBottomLevelAS != nullptr);

    FAccelerationStructureTLASParams TLASParams;
    TLASParams.pAccelerationStructures = pBottomLevelAS;

    pTopLevelAS = FAccelerationStructure::CreateTLAS(FApplication::Get().GetDevice(), TLASParams);
    assert(pTopLevelAS != nullptr);
}