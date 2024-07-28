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
}

void FRayTracer::ReleaseResources()
{
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