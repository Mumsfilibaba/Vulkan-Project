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

    static Application* Create();
    static Application& Get() { return *s_pInstance; }
private:
    Application();
    ~Application();
    DECL_NO_COPY(Application);

    void CreateWindow();
    void OnWindowClose();
private:
    GLFWwindow* m_pWindow;
    VulkanContext* m_pContext;
    bool m_bIsRunning;

    static Application* s_pInstance;
};