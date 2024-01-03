#pragma once
#include "Vulkan/VulkanContext.h"
#include "Renderer/IRenderer.h"

extern bool GIsRunning;

inline bool IsApplicationRunning()
{
    return GIsRunning;
}

inline void StartApplicationLoop()
{
    GIsRunning = true;
}

class Application
{
public:
    static Application* Create();
    
    static Application& Get()
    {
        return *AppInstance;
    }
    
    static GLFWwindow* GetWindow()
    {
        return AppInstance->m_pWindow;
    }

    Application();
    ~Application();

    bool Init();
    
    void Tick();
    
    void Release();
    
    GLFWwindow* CreateWindow();
    
    void OnWindowResize(GLFWwindow* pWindow, uint32_t width, uint32_t height);
    void OnWindowClose(GLFWwindow* pWindow);

    VulkanContext* GetVulkanContext() const
    {
        return m_pContext;
    }

private:
    GLFWwindow*    m_pWindow;
    IRenderer*     m_pRenderer;
    VulkanContext* m_pContext;
    
    uint32_t m_Width;
    uint32_t m_Height;
    
    std::chrono::time_point<std::chrono::system_clock> m_LastTime;

    static Application* AppInstance;
};
