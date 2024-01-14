#include "Application.h"
#include "Renderer/RayTracer.h"
#include "Renderer/GUI.h"

extern bool GIsRunning = false;

FApplication* FApplication::AppInstance = nullptr;

FApplication* FApplication::Create()
{
    AppInstance = new FApplication();
    return AppInstance;
}

FApplication::FApplication()
    : m_pWindow(nullptr)
    , m_pDevice(nullptr)
    , m_Width(1440)
    , m_Height(900)
{
}

FApplication::~FApplication()
{
}

bool FApplication::Init()
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

    if (!CreateWindow())
    {
        return false;
    }

    if (!glfwVulkanSupported())
    {
        std::cout << "GLFW: Vulkan Not Supported\n";
        return 1;
    }
    
    // Init Vulkan
    FDeviceParams params;
    params.pWindow           = m_pWindow;
    params.bEnableRayTracing = true;
    params.bEnableValidation = true;
    params.bVerbose          = false;

    m_pDevice = FDevice::Create(params);
    if (!m_pDevice)
    {
        std::cout << "Failed to init Vulkan\n";
        return false;
    }
    
    // Create Swapchain
    m_pSwapchain = FSwapchain::Create(m_pDevice, m_pWindow);

    // Initialize ImGui
    GUI::InitializeImgui(m_pWindow, m_pDevice, m_pSwapchain);

    m_pRenderer = new FRayTracer();
    m_pRenderer->Init(m_pDevice, m_pSwapchain);
    
    // Show window
    glfwShowWindow(m_pWindow);
        
    m_LastTime = std::chrono::system_clock::now();
    return true;
}

bool FApplication::CreateWindow()
{
    // Setup window
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Create window
    m_pWindow = glfwCreateWindow(m_Width, m_Height, "Vulkan Project", nullptr, nullptr);
    if (m_pWindow)
    {
        // Setup callbacks
        glfwSetWindowCloseCallback(m_pWindow, [](GLFWwindow* pWindow)
        {
            AppInstance->OnWindowClose(pWindow);
        });

        glfwSetWindowSizeCallback(m_pWindow, [](GLFWwindow* pWindow, int32_t width, int32_t height)
        {
            AppInstance->OnWindowResize(pWindow, width, height);
        });

        return true;
    }
    else
    {
        return false;
    }
}

void FApplication::OnWindowResize(GLFWwindow* pWindow, uint32_t width, uint32_t height)
{
    m_Width  = width;
    m_Height = height;

    // Resize the swapchain
    m_pSwapchain->Resize(width, m_Height);

    // Ensure that ImGui can create necessary resources for the main window
    GUI::OnSwapchainRecreated();

    // Let the renderer know about the resize
    m_pRenderer->OnWindowResize(m_Width, m_Height);
}

void FApplication::OnWindowClose(GLFWwindow* pWindow)
{
    if (pWindow == m_pWindow)
    {
        GIsRunning = false;
    }
}

void FApplication::Tick()
{
    auto currentTime = std::chrono::system_clock::now();
    
    // Update events
    glfwPollEvents();

    std::chrono::duration<double> elapsedSeconds = currentTime - m_LastTime;
    
    // Update GUI
    GUI::TickImGui();
    
    // Render
    m_pRenderer->Tick(elapsedSeconds.count());
    
    // Render the renderers UI
    m_pRenderer->OnRenderUI();
    
    // Render ImGui
    GUI::RenderImGui();
    
    // Present main window
    VkResult result = m_pSwapchain->Present();
    if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        GUI::OnSwapchainRecreated();
    }

    m_LastTime = currentTime;
}

void FApplication::Release()
{
    m_pDevice->WaitForIdle();

    m_pRenderer->Release();

    GUI::ReleaseImGui();

    SAFE_DELETE(m_pSwapchain);
    m_pDevice->Destroy();

    glfwDestroyWindow(m_pWindow);
    glfwTerminate();
    delete this;
}
