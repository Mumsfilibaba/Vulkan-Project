#pragma once
#include "VulkanContext.h"

#include <GLFW/glfw3.h>

class Application
{
public:
    DECL_NO_COPY(Application);

    bool IsRunning() { return m_bIsRunning; } 
    void Init();
    void Run();
    void Release();
    
    VulkanContext* GetVulkanContext() const { return m_pContext; }

    static Application* Create();
    static Application& Get() { return *s_pInstance; }
private:
    Application();
    ~Application();

    void CreateWindow();
    void OnWindowClose();
private:
    GLFWwindow* m_pWindow;
    VulkanContext* m_pContext;
    VulkanRenderPass* m_pRenderPass;
    VulkanGraphicsPipelineState* m_PipelineState;
    std::vector<VulkanFramebuffer*> m_Framebuffers;
    std::vector<VulkanCommandBuffer*> m_CommandBuffers;
    bool m_bIsRunning;

    static Application* s_pInstance;
};
