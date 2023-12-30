#include "Application.h"
#include "Renderer/RayTracer.h"

Application* Application::s_pInstance = nullptr;

Application* Application::Create()
{
    s_pInstance = new Application();
    return s_pInstance;
}

Application::Application()
    : m_pWindow(nullptr)
    , m_pContext(nullptr)
    , m_Width(1440)
    , m_Height(900)
    , m_bIsRunning(false)
{
}

Application::~Application()
{
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
    DeviceParams params = {};
    params.pWindow           = m_pWindow;
    params.bEnableRayTracing = true;
    params.bEnableValidation = true;
    params.bVerbose          = false;

    m_pContext = VulkanContext::Create(params);
    if (!m_pContext)
    {
        std::cout << "Failed to init Vulkan" << std::endl;
        return;
    }
    
    m_pRenderer = new RayTracer();
    m_pRenderer->Init(m_pContext);
    
    //Show window and start loop
    glfwShowWindow(m_pWindow);
    m_bIsRunning = true;
    
    m_LastTime = std::chrono::system_clock::now();
}

void Application::CreateWindow()
{
    //Setup error
    glfwSetErrorCallback([](int32_t, const char* pErrorMessage)
    {
        std::cerr << pErrorMessage << std::endl;
    });

    //Setup window
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GL_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    //Create window
    m_pWindow = glfwCreateWindow(m_Width, m_Height, "Vulkan Project", nullptr, nullptr);
    if (m_pWindow)
    {
        //Setup callbacks
        glfwSetWindowCloseCallback(m_pWindow, [](GLFWwindow*)
            {
                Application::Get().OnWindowClose();
            });

        glfwSetWindowSizeCallback(m_pWindow, [](GLFWwindow*, int32_t width, int32_t height)
            {
                Application::Get().OnWindowResize(width, height);
            });
    }
}

void Application::OnWindowResize(uint32_t width, uint32_t height)
{
    //Perform flush on commandbuffer
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    m_pContext->ExecuteGraphics(nullptr, waitStages);
    m_pContext->WaitForIdle();
    
    m_Width  = width;
    m_Height = height;
        
    m_pContext->ResizeBuffers(m_Width, m_Height);
    m_pRenderer->OnWindowResize(m_Width, m_Height);
}

void Application::OnWindowClose()
{
    m_bIsRunning = false;
}

void Application::Tick()
{
    auto currentTime = std::chrono::system_clock::now();
    glfwPollEvents();

    std::chrono::duration<double> elapsed_seconds = currentTime - m_LastTime;
    m_pRenderer->Tick(elapsed_seconds.count());
    m_pContext->Present();
    
    m_LastTime = currentTime;
}

void Application::Release()
{
    m_pContext->WaitForIdle();

    m_pRenderer->Release();
    m_pContext->Destroy();

    glfwDestroyWindow(m_pWindow);
    glfwTerminate();

    delete this;
}
