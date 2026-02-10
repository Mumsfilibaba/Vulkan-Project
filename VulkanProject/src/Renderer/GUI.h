#pragma once
#include "Core.h"
#include <ImGui/ImGui.h>

class CDevice;
class CDescriptorSet;
class CTextureView;
class CSwapchain;

namespace GUI
{
    void InitializeImgui(GLFWwindow* pWindow, CDevice* pDevice, CSwapchain* pSwapchain);
    void TickImGui();
    void RenderImGui();
    void ReleaseImGui();

    void OnSwapchainRecreated();

    CDescriptorSet* AllocateTextureID(CTextureView* pTextureView);
}
