#pragma once
#include "Vulkan/VulkanContext.h"

#include "Renderer/IRenderer.h"

class Application
{
public:
    inline bool IsRunning()
	{
		return m_bIsRunning;
	}
	
    void Init();
    void Run();
    void Release();
    
    inline VulkanContext* GetVulkanContext() const
	{
		return m_pContext;
	}

    static Application* Create();
    inline static Application& Get()
	{
		return *s_pInstance;
	}
	
	inline static GLFWwindow* GetWindow()
	{
		return s_pInstance->m_pWindow;
	}
	
private:
    Application();
    ~Application();

    void CreateWindow();

    void OnWindowResize(uint32_t width, uint32_t height);
    void OnWindowClose();
	
private:
    GLFWwindow* m_pWindow;
	IRenderer* m_pRenderer;
    VulkanContext* m_pContext;
	
    uint32_t m_Width;
    uint32_t m_Height;
    bool m_bIsRunning;
	
	std::chrono::time_point<std::chrono::system_clock> m_LastTime;

    static Application* s_pInstance;
};
