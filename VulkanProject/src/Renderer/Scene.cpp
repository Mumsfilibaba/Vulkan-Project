#include "Scene.h"
#include "Model.h"
#include "Application.h"

FScene::FScene()
    : m_Camera()
    , pBLAS(nullptr)
{
}

FScene::~FScene()
{
    if (FDevice* pDevice = FApplication::Get().GetDevice())
    {
        pDevice->WaitForIdle();
    }

    SAFE_DELETE(pBLAS);
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

    pBLAS = FAccelerationStructure::CreateBLAS(FApplication::Get().GetDevice(), BLASParams);
    assert(pBLAS != nullptr);
    return;
}