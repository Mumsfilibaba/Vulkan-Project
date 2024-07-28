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
}

void FRayTracer::Release()
{
    FBaseRenderer::Release();
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