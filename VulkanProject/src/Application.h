#pragma once
#include "Renderer/IRenderer.h"
#include "Vulkan/Device.h"
#include "Vulkan/Swapchain.h"

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
    
    bool CreateWindow();
    
    void OnWindowResize(GLFWwindow* pWindow, uint32_t width, uint32_t height);
    void OnWindowClose(GLFWwindow* pWindow);

    Device* GetVulkanContext() const
    {
        return m_pDevice;
    }

private:
    GLFWwindow* m_pWindow;
    IRenderer*  m_pRenderer;
    Device*     m_pDevice;
    Swapchain*  m_pSwapchain;
    
    uint32_t m_Width;
    uint32_t m_Height;
    
    std::chrono::time_point<std::chrono::system_clock> m_LastTime;

    static Application* AppInstance;
};
