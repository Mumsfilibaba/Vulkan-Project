#include "Application.h"

#include "Vulkan/VulkanBuffer.h"
#include "Vulkan/VulkanRenderPass.h"
#include "Vulkan/VulkanFramebuffer.h"
#include "Vulkan/VulkanShaderModule.h"
#include "Vulkan/VulkanPipelineState.h"
#include "Vulkan/VulkanCommandBuffer.h"
#include "Vulkan/VulkanDeviceAllocator.h"

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
    m_pCurrentCommandBuffer(nullptr),
    m_pVertexBuffer(nullptr),
    m_pIndexBuffer(nullptr),
    m_pDeviceAllocator(nullptr),
    m_Width(1440),
    m_Height(900),
    m_CommandBuffers(),
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
    params.bVerbose = false;

    m_pContext = VulkanContext::Create(params);
    if (!m_pContext)
    {
        std::cout << "Failed to init Vulkan" << std::endl;
        return;
    }
    
    //PipelineState, RenderPass and Shaders
    VulkanShaderModule* pVertex   = VulkanShaderModule::CreateFromFile(m_pContext, "main", "res/shaders/vertex.spv");
    VulkanShaderModule* pFragment = VulkanShaderModule::CreateFromFile(m_pContext, "main", "res/shaders/fragment.spv");

    RenderPassAttachment attachments[1]; 
    attachments[0].Format = m_pContext->GetSwapChainFormat();

    RenderPassParams renderPassParams = {};
    renderPassParams.ColorAttachmentCount = 1;
    renderPassParams.pColorAttachments = attachments;
    m_pRenderPass = m_pContext->CreateRenderPass(renderPassParams);

    VkVertexInputBindingDescription bindingDescription = Vertex::GetBindingDescription();

    GraphicsPipelineStateParams pipelineParams = {};
    pipelineParams.pBindingDescriptions = &bindingDescription;
    pipelineParams.BindingDescriptionCount = 1;
    pipelineParams.pAttributeDescriptions = Vertex::GetAttributeDescriptions();
    pipelineParams.AttributeDescriptionCount = 2;
    pipelineParams.pVertex = pVertex;
    pipelineParams.pFragment = pFragment;
    pipelineParams.pRenderPass = m_pRenderPass;
    m_PipelineState = m_pContext->CreateGraphicsPipelineState(pipelineParams);

    delete pVertex;
    delete pFragment;
   
    //Framebuffers
    CreateFramebuffers();

    //Commandbuffers
    CommandBufferParams commandBufferParams = {};
    commandBufferParams.Level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferParams.QueueType   = ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS;

    uint32_t imageCount = m_pContext->GetImageCount();
    m_CommandBuffers.resize(imageCount);
    for (size_t i = 0; i < m_CommandBuffers.size(); i++)
    {
        VulkanCommandBuffer* pCommandBuffer = m_pContext->CreateCommandBuffer(commandBufferParams);
        m_CommandBuffers[i] = pCommandBuffer;
    }

    //Allocator for GPU mem
    m_pDeviceAllocator = m_pContext->CreateDeviceAllocator();

    //Vertex- and indexbuffers
    const std::vector<Vertex> vertices = 
    {
        { {-0.5f, -0.5f},   {1.0f, 0.0f, 0.0f} },
        { {0.5f, -0.5f},    {0.0f, 1.0f, 0.0f} },
        { {0.5f, 0.5f},     {0.0f, 0.0f, 1.0f} },
        { {-0.5f, 0.5f},    {1.0f, 1.0f, 1.0f} }
    };

    BufferParams vertexBufferParams = {};
    vertexBufferParams.SizeInBytes = vertices.size() * sizeof(Vertex);
    vertexBufferParams.Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vertexBufferParams.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    m_pVertexBuffer = m_pContext->CreateBuffer(vertexBufferParams, m_pDeviceAllocator);

    void* pCPUMem = nullptr;
    m_pVertexBuffer->Map(&pCPUMem);
    memcpy(pCPUMem, vertices.data(), vertexBufferParams.SizeInBytes);
    m_pVertexBuffer->Unmap();

    const std::vector<uint16_t> indices =
    {
        0, 1, 2, 
        2, 3, 0
    };

    BufferParams indexBufferParams = {};
    indexBufferParams.SizeInBytes = indices.size() * sizeof(uint16_t);
    indexBufferParams.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    indexBufferParams.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    m_pIndexBuffer = m_pContext->CreateBuffer(indexBufferParams, nullptr);

    pCPUMem = nullptr;
    m_pIndexBuffer->Map(&pCPUMem);
    memcpy(pCPUMem, indices.data(), indexBufferParams.SizeInBytes);
    m_pIndexBuffer->Unmap();

    //Show window and start loop
    glfwShowWindow(m_pWindow);
    m_bIsRunning = true;
}

void Application::CreateWindow()
{
    //Setup error
    glfwSetErrorCallback([](int32_t, const char* pErrorMessage)
    {
        std::cerr << pErrorMessage << std::endl;
    });

    //Setup window
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GL_TRUE);
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

void Application::CreateFramebuffers()
{
    uint32_t imageCount = m_pContext->GetImageCount();
    m_Framebuffers.resize(imageCount);

    VkExtent2D extent = m_pContext->GetFramebufferExtent();
    
    FramebufferParams framebufferParams = {};
    framebufferParams.AttachMentCount   = 1;
    framebufferParams.Width             = extent.width;
    framebufferParams.Height            = extent.height;
    framebufferParams.pRenderPass       = m_pRenderPass;

    for (size_t i = 0; i < m_Framebuffers.size(); i++)
    {
        VkImageView imageView = m_pContext->GetSwapChainImageView(uint32_t(i));
        framebufferParams.pAttachMents = &imageView;
        m_Framebuffers[i] = m_pContext->CreateFrameBuffer(framebufferParams);
    }
}

void Application::ReleaseFramebuffers()
{
    for (auto& framebuffer : m_Framebuffers)
    {
        delete framebuffer;
    }

    m_Framebuffers.clear();
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

    ReleaseFramebuffers();
    CreateFramebuffers();
}

void Application::OnWindowClose()
{
    m_bIsRunning = false;
}

void Application::Run()
{
    glfwPollEvents();

    uint32_t frameIndex = m_pContext->GetCurrentBackBufferIndex();
    m_pCurrentCommandBuffer = m_CommandBuffers[frameIndex];

    m_pCurrentCommandBuffer->Reset();
    m_pCurrentCommandBuffer->Begin();

    VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_pCurrentCommandBuffer->BeginRenderPass(m_pRenderPass, m_Framebuffers[frameIndex], &clearColor, 1);
    
    VkExtent2D extent = m_pContext->GetFramebufferExtent();
    VkViewport viewport = { 0.0f, 0.0f, float(extent.width), float(extent.height), 0.0f, 1.0f };
    m_pCurrentCommandBuffer->SetViewport(viewport);
    
    VkRect2D scissor = { { 0, 0}, extent };
    m_pCurrentCommandBuffer->SetScissorRect(scissor);
    
    m_pCurrentCommandBuffer->BindGraphicsPipelineState(m_PipelineState);
    m_pCurrentCommandBuffer->BindVertexBuffer(m_pVertexBuffer, 0, 0);
    m_pCurrentCommandBuffer->BindIndexBuffer(m_pIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

    m_pCurrentCommandBuffer->DrawIndexInstanced(6, 1, 0, 0, 0);
    
    m_pCurrentCommandBuffer->EndRenderPass();

    m_pCurrentCommandBuffer->End();

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    m_pContext->ExecuteGraphics(m_pCurrentCommandBuffer, waitStages);
    m_pContext->Present();
}

void Application::Release()
{
    m_pContext->WaitForIdle();

	delete m_pIndexBuffer;
	delete m_pVertexBuffer;

    for (auto& commandBuffer : m_CommandBuffers)
    {
        delete commandBuffer;
    }
    m_CommandBuffers.clear();

    ReleaseFramebuffers();

	delete m_pRenderPass;
	delete m_PipelineState;

	delete m_pDeviceAllocator;
    m_pContext->Destroy();

    glfwDestroyWindow(m_pWindow);
    glfwTerminate();

    delete this;
}
