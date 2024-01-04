#pragma once
#include "Core.h"
#include <ImGui/ImGui.h>

class Device;
class DescriptorSet;
class TextureView;
class Swapchain;

namespace GUI
{
    void InitializeImgui(GLFWwindow* pWindow, Device* pDevice, Swapchain* pSwapchain);
    
    void TickImGui();
    
    void RenderImGui();
    
    void ReleaseImGui();

    void OnSwapchainRecreated();
    
    DescriptorSet* AllocateTextureID(TextureView* pTextureView);
}
