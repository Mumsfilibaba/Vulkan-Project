#pragma once
#include "Core.h"
#include <ImGui/ImGui.h>

class VulkanContext;
class DescriptorSet;
class TextureView;

namespace ImGuiRenderer
{
    void InitializeImgui(GLFWwindow* pWindow, VulkanContext* pContext);
    
    void TickImGui();
    
    void RenderImGui();
    
    void ReleaseImGui();
    
    DescriptorSet* AllocateTextureID(TextureView* pTextureView);
}
