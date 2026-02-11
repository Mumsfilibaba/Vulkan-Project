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

class CApplication
{
public:
    static CApplication* Create();

    static CApplication& Get()
    {
        assert(ApplicationInstance != nullptr);
        return *ApplicationInstance;
    }

    CApplication();
    ~CApplication();

    bool Init();
    void Tick();
    void Release();
    bool CreateWindow();

    void OnWindowMinimized(GLFWwindow* pWindow, int32_t Minimized);
    void OnWindowResize(GLFWwindow* pWindow, int32_t width, int32_t height);
    void OnWindowClose(GLFWwindow* pWindow);

    GLFWwindow* GetWindow()
    {
        return m_pWindow;
    }

    CDevice* GetDevice() const
    {
        return m_pDevice;
    }

private:
    GLFWwindow* m_pWindow;
    IRenderer*  m_pRenderer;
    CDevice*    m_pDevice;
    CSwapchain* m_pSwapchain;
    uint32_t    m_Width;
    uint32_t    m_Height;
    bool        m_bIsMinimized;

    std::chrono::time_point<std::chrono::system_clock> m_LastTime;

    static CApplication* ApplicationInstance;
};
