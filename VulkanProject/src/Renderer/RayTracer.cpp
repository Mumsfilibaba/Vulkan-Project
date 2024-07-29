#include "RayTracer.h"
#include "GUI.h"

FRayTracer::FRayTracer()
    : FBaseRenderer()
{
}

FRayTracer::~FRayTracer()
{
}

void FRayTracer::CreateResources()
{
    m_pScene = new FScene();
    m_pScene->Initialize();
}

void FRayTracer::ReleaseResources()
{
    SAFE_DELETE(m_pScene);
}

void FRayTracer::Render(FCommandBuffer* pCommandBuffer)
{
}

void FRayTracer::RenderSceneUI()
{
}

void FRayTracer::ReloadShaders()
{
}