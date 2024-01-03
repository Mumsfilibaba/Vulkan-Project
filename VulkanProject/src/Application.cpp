#include "Application.h"
#include "Renderer/RayTracer.h"
#include "Renderer/ImGuiRenderer.h"

extern bool GIsRunning = false;

Application* Application::AppInstance = nullptr;

Application* Application::Create()
{
    AppInstance = new Application();
    return AppInstance;
}

Application::Application()
    : m_pWindow(nullptr)
    , m_pContext(nullptr)
    , m_Width(1440)
    , m_Height(900)
{
}

Application::~Application()
{
}

bool Application::Init()
{
    // Setup error handling
    glfwSetErrorCallback([](int32_t, const char* pErrorMessage)
    {
        std::cerr << pErrorMessage << '\n';
    });
    
    // Init window library
    if (!glfwInit())
    {
        std::cout << "Failed to init GLFW\n";
        return false;
    }

    m_pWindow = CreateWindow();
    if (!m_pWindow)
    {
        return false;
    }

    if (!glfwVulkanSupported())
    {
        std::cout << "GLFW: Vulkan Not Supported\n";
        return 1;
    }
    
    // Init vulkan
    DeviceParams params;
    params.pWindow           = m_pWindow;
    params.bEnableRayTracing = true;
    params.bEnableValidation = true;
    params.bVerbose          = false;

    m_pContext = VulkanContext::Create(params);
    if (!m_pContext)
    {
        std::cout << "Failed to init Vulkan\n";
        return false;
    }
    
    // Initialize ImGui
    ImGuiRenderer::InitializeImgui(m_pWindow, m_pContext);

    m_pRenderer = new RayTracer();
    m_pRenderer->Init(m_pContext);
    
    // Show window
    glfwShowWindow(m_pWindow);
        
    m_LastTime = std::chrono::system_clock::now();
    return true;
}

GLFWwindow* Application::CreateWindow()
{
    // Setup window
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Create window
    GLFWwindow* pWindow = glfwCreateWindow(m_Width, m_Height, "Vulkan Project", nullptr, nullptr);
    if (pWindow)
    {
        // Setup callbacks
        glfwSetWindowCloseCallback(pWindow, [](GLFWwindow* pWindow)
        {
            AppInstance->OnWindowClose(pWindow);
        });

        glfwSetWindowSizeCallback(pWindow, [](GLFWwindow* pWindow, int32_t width, int32_t height)
        {
            AppInstance->OnWindowResize(pWindow, width, height);
        });
    }
    
    return pWindow;
}

void Application::OnWindowResize(GLFWwindow* pWindow, uint32_t width, uint32_t height)
{
    // Perform flush on commandbuffer
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    m_pContext->ExecuteGraphics(nullptr, m_pContext->GetSwapChain(), waitStages);
    m_pContext->WaitForIdle();
    
    m_Width  = width;
    m_Height = height;
        
    m_pContext->ResizeBuffers(m_Width, m_Height);
    m_pRenderer->OnWindowResize(m_Width, m_Height);
}

void Application::OnWindowClose(GLFWwindow* pWindow)
{
    if (pWindow == m_pWindow)
    {
        GIsRunning = false;
    }
}

void Application::Tick()
{
    auto currentTime = std::chrono::system_clock::now();
    
    // Update events
    glfwPollEvents();

    std::chrono::duration<double> elapsedSeconds = currentTime - m_LastTime;
    
    // Update GUI
    ImGuiRenderer::TickImGui();
    
    // Render
    m_pRenderer->Tick(elapsedSeconds.count());
    
    // Render the renderers UI
    m_pRenderer->OnRenderUI();
    
    // Render ImGui
    ImGuiRenderer::RenderImGui();
    
    // Update screen
    m_pContext->Present();
    
    m_LastTime = currentTime;
}

void Application::Release()
{
    m_pContext->WaitForIdle();

    ImGuiRenderer::ReleaseImGui();
    
    m_pRenderer->Release();
    m_pContext->Destroy();

    glfwDestroyWindow(m_pWindow);
    glfwTerminate();
    delete this;
}
