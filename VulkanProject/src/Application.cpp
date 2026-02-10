#include "Application.h"
#include "Renderer/SoftwareRayTracer.h"
#include "Renderer/GUI.h"
#include "Renderer/RayTracer.h"

#define ENABLE_HW_RT 1

extern bool GIsRunning = false;

CApplication* CApplication::GAppInstance = nullptr;

CApplication* CApplication::Create()
{
    GAppInstance = new CApplication();
    return GAppInstance;
}

CApplication::CApplication()
    : m_pWindow(nullptr)
    , m_pDevice(nullptr)
    , m_Width(1440)
    , m_Height(900)
{
}

CApplication::~CApplication()
{
}

bool CApplication::Init()
{
    // Setup error handling
    glfwSetErrorCallback([](int32_t, const char* pErrorMessage)
    {
        LOG("%s\n", pErrorMessage);
    });
    
    // Init window library
    if (!glfwInit())
    {
        LOG("Failed to init GLFW\n");
        return false;
    }

    if (!CreateWindow())
    {
        return false;
    }

    if (!glfwVulkanSupported())
    {
        LOG("GLFW: Vulkan Not Supported\n");
        return 1;
    }
    
    // Init Vulkan
    SDeviceParams DeviceParams;
    DeviceParams.pWindow           = m_pWindow;
    DeviceParams.bEnableRayTracing = true;
    DeviceParams.bEnableValidation = true;
    DeviceParams.bVerbose          = false;

    m_pDevice = CDevice::Create(DeviceParams);
    if (!m_pDevice)
    {
        LOG("Failed to init Vulkan\n");
        return false;
    }
    
    // Create SwapChain
    m_pSwapchain = CSwapchain::Create(m_pDevice, m_pWindow);

    // Initialize ImGui
    GUI::InitializeImgui(m_pWindow, m_pDevice, m_pSwapchain);

#if ENABLE_HW_RT
    if (m_pDevice->IsRayTracingSupported())
    {
        m_pRenderer = new CRayTracer();
    }
    else
#endif
    {
        m_pRenderer = new CSoftwareRayTracer();
    }

    m_pRenderer->Init(m_pDevice, m_pSwapchain);
    
    // Show window
    glfwShowWindow(m_pWindow);
    m_bIsMinimized = false;

    m_LastTime = std::chrono::system_clock::now();
    return true;
}

bool CApplication::CreateWindow()
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
        glfwSetWindowIconifyCallback(m_pWindow, [](GLFWwindow* pWindow, int32_t Minimized)
        {
            GAppInstance->OnWindowMinimized(pWindow, Minimized);
        });

        glfwSetWindowCloseCallback(m_pWindow, [](GLFWwindow* pWindow)
        {
            GAppInstance->OnWindowClose(pWindow);
        });

        glfwSetWindowSizeCallback(m_pWindow, [](GLFWwindow* pWindow, int32_t Width, int32_t Height)
        {
            GAppInstance->OnWindowResize(pWindow, Width, Height);
        });

        return true;
    }
    else
    {
        return false;
    }
}

void CApplication::OnWindowMinimized(GLFWwindow* pWindow, int32_t Minimized)
{
    if (Minimized)
    {
        m_bIsMinimized = true;
    }
    else
    {
        m_bIsMinimized = false;
    }
}

void CApplication::OnWindowResize(GLFWwindow* pWindow, uint32_t Width, uint32_t Height)
{
    // This happens when we minimize a window
    if (Width > 0 && Height > 0)
    {
        m_Width  = Width;
        m_Height = Height;

        // Resize the swapchain
        m_pSwapchain->Resize(Width, m_Height);

        // Ensure that ImGui can create necessary resources for the main window
        GUI::OnSwapchainRecreated();
    }
}

void CApplication::OnWindowClose(GLFWwindow* pWindow)
{
    if (pWindow == m_pWindow)
    {
        GIsRunning = false;
    }
}

void CApplication::Tick()
{
    auto CurrentTime = std::chrono::system_clock::now();
    
    // Update events
    glfwPollEvents();

    std::chrono::duration<double> ElapsedSeconds = CurrentTime - m_LastTime;

    // Skip rendering if we are minimized
    if (!m_bIsMinimized)
    {
        // Update GUI
        GUI::TickImGui();
    
        // Render
        m_pRenderer->Tick(ElapsedSeconds.count());
    
        // Render the renderers UI
        m_pRenderer->OnRenderUI();
    
        // Render ImGui
        GUI::RenderImGui();
    
        // Present main window
        VkResult Result = m_pSwapchain->Present();
        if (Result == VK_SUBOPTIMAL_KHR || Result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            GUI::OnSwapchainRecreated();
        }
    }

    m_LastTime = CurrentTime;
}

void CApplication::Release()
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
