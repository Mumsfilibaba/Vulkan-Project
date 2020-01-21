#include "Application.h"
#include <iostream>

Application* Application::s_pInstance = nullptr;

Application::Application()
{
}

Application::~Application()
{
}

Application* Application::Create()
{
    s_pInstance = new Application();
    return s_pInstance;
}

void Application::Init()
{
    //Open window
    if (glfwInit())
    {
        CreateWindow();
    }
    else
    {
        std::cout << "Failed to init GLFW" << std::endl;
        return;
    }

#ifdef _DEBUG
    bool debug = true;
#else
    bool debug = false;
#endif
    //Init vulkan
    m_pContext = new VulkanContext();
    if (!m_pContext->Init(m_pWindow, true))
    {
        return;
    }
    
    //Show window and start loop
    glfwShowWindow(m_pWindow);
    m_bIsRunning = true;
}

void Application::CreateWindow()
{
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_pWindow = glfwCreateWindow(1440, 900, "Vulkan Project", nullptr, nullptr);
    if (m_pWindow)
    {
        glfwSetWindowCloseCallback(m_pWindow, [](GLFWwindow* window)
		{
            Application::Get().OnWindowClose();
		});
    }
}

void Application::Run()
{
    glfwPollEvents();
}

void Application::OnWindowClose()
{
    m_bIsRunning = false;
}

void Application::Release()
{
    glfwDestroyWindow(m_pWindow);
    glfwTerminate();
    delete this;
}
