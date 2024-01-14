#pragma once
#include "Core.h"
#include <ImGui/ImGui.h>

class FDevice;
class FDescriptorSet;
class FTextureView;
class FSwapchain;

namespace GUI
{
    void InitializeImgui(GLFWwindow* pWindow, FDevice* pDevice, FSwapchain* pSwapchain);
    
    void TickImGui();
    
    void RenderImGui();
    
    void ReleaseImGui();

    void OnSwapchainRecreated();
    
    FDescriptorSet* AllocateTextureID(FTextureView* pTextureView);
}
