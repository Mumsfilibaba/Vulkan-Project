#include "RayTracer.h"
#include "GUI.h"
#include "Scene.h"

FRayTracer::FRayTracer()
    : FBaseRenderer()
{
}

FRayTracer::~FRayTracer()
{
}

void FRayTracer::Init(FDevice* pDevice, FSwapchain* pSwapchain)
{
    FBaseRenderer::Init(pDevice, pSwapchain);
}

void FRayTracer::Release()
{
    FBaseRenderer::Release();
}

void FRayTracer::RenderSceneUI()
{
}

void FRayTracer::ReloadShaders()
{
}