#include "Application.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanShaderModule.h"
#include "VulkanPipelineState.h"
#include "VulkanCommandBuffer.h"

#include <iostream>

Application* Application::s_pInstance = nullptr;

Application* Application::Create()
{
    s_pInstance = new Application();
    return s_pInstance;
}

Application::Application()
    : m_pWindow(nullptr),
    m_pContext(nullptr),
    m_pRenderPass(nullptr),
    m_PipelineState(nullptr),
    m_Width(1440),
    m_Height(900),
    m_Framebuffers(),
    m_bIsRunning(false)
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
    params.pWindow = m_pWindow;
    params.bEnableRayTracing = true;
    params.bEnableValidation = true;

    m_pContext = VulkanContext::Create(params);
    if (!m_pContext)
    {
        std::cout << "Failed to init Vulkan" << std::endl;
        return;
    }
    
    VulkanShaderModule* pVertex   = VulkanShaderModule::CreateFromFile(m_pContext, "main", "res/shaders/vertex.spv");
    VulkanShaderModule* pFragment = VulkanShaderModule::CreateFromFile(m_pContext, "main", "res/shaders/fragment.spv");

    RenderPassAttachment attachments[1]; 
    attachments[0].Format = m_pContext->GetSwapChainFormat();

    RenderPassParams renderPassParams = {};
    renderPassParams.ColorAttachmentCount = 1;
    renderPassParams.pColorAttachments = attachments;
    m_pRenderPass = m_pContext->CreateRenderPass(renderPassParams);

    GraphicsPipelineStateParams pipelineParams = {};
    pipelineParams.pVertex = pVertex;
    pipelineParams.pFragment = pFragment;
    pipelineParams.pRenderPass = m_pRenderPass;
    m_PipelineState = m_pContext->CreateGraphicsPipelineState(pipelineParams);

    delete pVertex;
    delete pFragment;

    FramebufferParams framebufferParams = {};
    framebufferParams.AttachMentCount = 1;

    VkExtent2D extent = m_pContext->GetFramebufferExtent();
    framebufferParams.Width  = extent.width;
    framebufferParams.Height = extent.height;
    framebufferParams.pRenderPass = m_pRenderPass;
    
    CommandBufferParams commandBufferParams = {};
    commandBufferParams.Level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferParams.QueueType   = ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS;

    uint32 imageCount = m_pContext->GetImageCount();
    m_Framebuffers.resize(imageCount);
    m_CommandBuffers.resize(imageCount);
    for (size_t i = 0; i < m_Framebuffers.size(); i++)
    {
        VkImageView imageView = m_pContext->GetSwapChainImageView(i);
        framebufferParams.pAttachMents = &imageView;
        m_Framebuffers[i]   = m_pContext->CreateFrameBuffer(framebufferParams);

        VulkanCommandBuffer* pCommandBuffer = m_pContext->CreateCommandBuffer(commandBufferParams);
        m_CommandBuffers[i] = pCommandBuffer;
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
    m_pWindow = glfwCreateWindow(m_Width, m_Height, "Vulkan Project", nullptr, nullptr);
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

    uint32 frameIndex = m_pContext->GetCurrentBackBufferIndex();
    VulkanCommandBuffer* pCurrentCommandBuffer = m_CommandBuffers[frameIndex];
    pCurrentCommandBuffer->Reset();
    pCurrentCommandBuffer->Begin();

    VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    pCurrentCommandBuffer->BeginRenderPass(m_pRenderPass, m_Framebuffers[frameIndex], &clearColor, 1);
    
    VkViewport viewport = { 0.0f, 0.0f, float(m_Width), float(m_Height), 0.0f, 1.0f };
    pCurrentCommandBuffer->SetViewport(viewport);
    
    VkRect2D scissor = { { 0, 0}, { m_Width, m_Height } };
    pCurrentCommandBuffer->SetScissorRect(scissor);
    
    pCurrentCommandBuffer->BindGraphicsPipelineState(m_PipelineState);
    pCurrentCommandBuffer->DrawInstanced(3, 1, 0, 0);
    
    pCurrentCommandBuffer->EndRenderPass();

    pCurrentCommandBuffer->End();

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    m_pContext->ExecuteGraphics(pCurrentCommandBuffer, waitStages);
    m_pContext->Present();
}

void Application::OnWindowClose()
{
    m_bIsRunning = false;
}

void Application::Release()
{
    m_pContext->WaitForIdle();
    for (auto& commandBuffer : m_CommandBuffers)
    {
        delete commandBuffer;
        commandBuffer = nullptr;
    }
    m_CommandBuffers.clear();

    for (auto& framebuffer : m_Framebuffers)
    {
        delete framebuffer;
        framebuffer = nullptr;
    }
    m_Framebuffers.clear();

    delete m_pRenderPass;
    delete m_PipelineState;

    m_pContext->Destroy();

    glfwDestroyWindow(m_pWindow);
    glfwTerminate();

    delete this;
}
