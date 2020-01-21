#pragma once
#include "VulkanContext.h"
#include <GLFW/glfw3.h>

class Application
{
public:
    bool IsRunning() { return m_bIsRunning; } 
    void Init();
    void Run();
    void Release();
    
    VulkanContext* GetVulkanContext() const { return m_pContext; }

    static Application* Create();
    static Application& Get() { return *s_pInstance; }
private:
    DECL_NO_COPY(Application);
    
    Application();
    ~Application();

    void CreateWindow();
    void OnWindowClose();
private:
    GLFWwindow* m_pWindow;
    VulkanContext* m_pContext;
    VulkanSwapChain* m_pSwapChain;
    bool m_bIsRunning;

    static Application* s_pInstance;
};
