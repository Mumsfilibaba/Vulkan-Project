#include "Application.h"
#include <iostream>

Application* Application::s_pInstance = nullptr;

Application::Application()
    : m_pWindow(nullptr),
    m_pContext(nullptr),
    m_pSwapChain(nullptr),
    m_bIsRunning(false)
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

    //Init vulkan
    ContextParams params = {};
    params.bEnableRayTracing = true;
    params.bEnableValidation = true;

    m_pContext = VulkanContext::Create(params);
    if (!m_pContext)
    {
        std::cout << "Failed to init Vulkan" << std::endl;
        return;
    }

    m_pSwapChain = m_pContext->CreateSwapChain(m_pWindow);
    if (m_pSwapChain)
    {
        std::cout << "Created swapchain" << std::endl;
    }
    else
    {
        std::cout << "Failed to create swapchain" << std::endl;
        return;
    }
    
    //Show window and start loop
    glfwShowWindow(m_pWindow);
    m_bIsRunning = true;
}

void Application::CreateWindow()
{
    //Setup error
    glfwSetErrorCallback([](int32, const char* pErrorMessage)
    {
        std::cerr << pErrorMessage << std::endl;
    });

    //Setup window
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    //Create window
    m_pWindow = glfwCreateWindow(1440, 900, "Vulkan Project", nullptr, nullptr);
    if (m_pWindow)
    {
        //Setup callbacks
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
    m_pContext->DestroySwapChain(&m_pSwapChain);
    m_pContext->Release();

    glfwDestroyWindow(m_pWindow);
    glfwTerminate();

    delete this;
}
