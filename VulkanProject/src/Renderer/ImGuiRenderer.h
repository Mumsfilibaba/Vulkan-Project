#pragma once
#include "Core.h"
#include <ImGui/ImGui.h>

class VulkanContext;

namespace ImGuiRenderer
{
    void InitializeImgui(GLFWwindow* pWindow, VulkanContext* pContext);
    
    void TickImGui();
    
    void RenderImGui();
    
    void ReleaseImGui();
}
