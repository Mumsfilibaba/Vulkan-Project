#pragma once
#include "Vulkan/VulkanContext.h"

#include "Renderer/Renderer.h"

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
	
private:
    Application();
    ~Application();

    void CreateWindow();

    void OnWindowResize(uint32_t width, uint32_t height);
    void OnWindowClose();
	
private:
    GLFWwindow* m_pWindow;
	Renderer* m_pRenderer;
    VulkanContext* m_pContext;
	
    uint32_t m_Width;
    uint32_t m_Height;
    bool m_bIsRunning;

    static Application* s_pInstance;
};
