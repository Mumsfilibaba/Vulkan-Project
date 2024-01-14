#include "Renderer.h"
#include "Model.h"
#include "Camera.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/RenderPass.h"
#include "Vulkan/Framebuffer.h"
#include "Vulkan/ShaderModule.h"
#include "Vulkan/PipelineState.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/DeviceMemoryAllocator.h"
#include "Vulkan/Swapchain.h"

FRenderer::FRenderer()
    : m_pDevice(nullptr)
    , m_pRenderPass(nullptr)
    , m_PipelineState(nullptr)
    , m_pCurrentCommandBuffer(nullptr)
    , m_pModel(nullptr)
    , m_pDeviceAllocator(nullptr)
    , m_CommandBuffers()
    , m_Framebuffers()
{
}

void FRenderer::Init(FDevice* pDevice, FSwapchain* pSwapchain)
{
    // Set device
    m_pDevice    = pDevice;
    m_pSwapchain = pSwapchain;
    
    // PipelineState, RenderPass and Shaders
    FShaderModule* pVertex   = FShaderModule::CreateFromFile(m_pDevice, "main", "res/shaders/vertex.spv");
    FShaderModule* pFragment = FShaderModule::CreateFromFile(m_pDevice, "main", "res/shaders/fragment.spv");

    FRenderPassAttachment attachments[1];
    attachments[0].Format = m_pSwapchain->GetFormat();

    FRenderPassParams renderPassParams = {};
    renderPassParams.ColorAttachmentCount = 1;
    renderPassParams.pColorAttachments    = attachments;
    m_pRenderPass = FRenderPass::Create(m_pDevice, renderPassParams);

    VkVertexInputBindingDescription bindingDescription = FVertex::GetBindingDescription();

    FGraphicsPipelineStateParams pipelineParams = {};
    pipelineParams.pBindingDescriptions      = &bindingDescription;
    pipelineParams.bindingDescriptionCount   = 1;
    pipelineParams.pAttributeDescriptions    = FVertex::GetAttributeDescriptions();
    pipelineParams.attributeDescriptionCount = 3;
    pipelineParams.pVertexShader             = pVertex;
    pipelineParams.pFragmentShader           = pFragment;
    pipelineParams.pRenderPass               = m_pRenderPass;
    m_PipelineState = FGraphicsPipeline::Create(m_pDevice, pipelineParams);

    delete pVertex;
    delete pFragment;
   
    // Framebuffers
    CreateFramebuffers();

    // CommandBuffers
    FCommandBufferParams commandBufferParams = {};
    commandBufferParams.Level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferParams.QueueType = ECommandQueueType::Graphics;

    uint32_t imageCount = m_pSwapchain->GetNumBackBuffers();
    m_CommandBuffers.resize(imageCount);
    for (size_t i = 0; i < m_CommandBuffers.size(); i++)
    {
        FCommandBuffer* pCommandBuffer = FCommandBuffer::Create(m_pDevice, commandBufferParams);
        m_CommandBuffers[i] = pCommandBuffer;
    }

    // Allocator for GPU mem
    m_pDeviceAllocator = new FDeviceMemoryAllocator(m_pDevice->GetDevice(), m_pDevice->GetPhysicalDevice());

    m_pModel = new FModel();
    m_pModel->LoadFromFile("res/models/viking_room.obj", m_pDevice, m_pDeviceAllocator);
    
    // Camera
    FBufferParams camBuffParams;
    camBuffParams.Size      = sizeof(FCameraBuffer);
    camBuffParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    camBuffParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    m_pCameraBuffer = FBuffer::Create(m_pDevice, camBuffParams, m_pDeviceAllocator);
    
    // Create descriptorpool
    FDescriptorPoolParams poolParams;
    poolParams.NumUniformBuffers = 1;
    poolParams.MaxSets           = 1;
    m_pDescriptorPool = FDescriptorPool::Create(m_pDevice, poolParams);
}

void FRenderer::Tick(float deltaTime)
{
    // Update
    VkExtent2D extent = m_pSwapchain->GetExtent();
    m_Camera.Update(90.0f, extent.width, extent.height, 0.1f, 100.0f);
    
    // Draw
    uint32_t frameIndex = m_pSwapchain->GetCurrentBackBufferIndex();
    m_pCurrentCommandBuffer = m_CommandBuffers[frameIndex];

    // Begin CommandBuffer
    m_pCurrentCommandBuffer->Reset();
    m_pCurrentCommandBuffer->Begin();

    // Update camera
    FCameraBuffer camBuff;
    camBuff.Projection = m_Camera.GetProjectionMatrix();
    camBuff.View       = m_Camera.GetViewMatrix();
    m_pCurrentCommandBuffer->UpdateBuffer(m_pCameraBuffer, 0, sizeof(FCameraBuffer), &camBuff);
    
    // Begin renderpass
    VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_pCurrentCommandBuffer->BeginRenderPass(m_pRenderPass, m_Framebuffers[frameIndex], &clearColor, 1);
    
    // Set viewport
    VkViewport viewport = { 0.0f, 0.0f, float(extent.width), float(extent.height), 0.0f, 1.0f };
    m_pCurrentCommandBuffer->SetViewport(viewport);
    VkRect2D scissor = { { 0, 0}, extent };
    m_pCurrentCommandBuffer->SetScissorRect(scissor);
    
    // Bind pipeline
    m_pCurrentCommandBuffer->BindGraphicsPipelineState(m_PipelineState);

    // Draw
    m_pCurrentCommandBuffer->BindVertexBuffer(m_pModel->GetVertexBuffer(), 0, 0);
    m_pCurrentCommandBuffer->BindIndexBuffer(m_pModel->GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);
    m_pCurrentCommandBuffer->DrawIndexInstanced(m_pModel->GetIndexCount(), 1, 0, 0, 0);
    
    // End renderpass
    m_pCurrentCommandBuffer->EndRenderPass();
    m_pCurrentCommandBuffer->End();

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    m_pDevice->ExecuteGraphics(m_pCurrentCommandBuffer, m_pSwapchain, waitStages);
}

void FRenderer::Release()
{
    delete m_pModel;

    for (auto& commandBuffer : m_CommandBuffers)
    {
        delete commandBuffer;
    }
    
    m_CommandBuffers.clear();

    ReleaseFramebuffers();

    delete m_pRenderPass;
    delete m_PipelineState;

    delete m_pDeviceAllocator;    
}

void FRenderer::OnWindowResize(uint32_t width, uint32_t height)
{
    ReleaseFramebuffers();
    CreateFramebuffers();
}

void FRenderer::CreateFramebuffers()
{
    uint32_t imageCount = m_pSwapchain->GetNumBackBuffers();
    m_Framebuffers.resize(imageCount);

    VkExtent2D extent = m_pSwapchain->GetExtent();
    
    FFramebufferParams framebufferParams = {};
    framebufferParams.AttachMentCount   = 1;
    framebufferParams.Width             = extent.width;
    framebufferParams.Height            = extent.height;
    framebufferParams.pRenderPass       = m_pRenderPass;

    for (size_t i = 0; i < m_Framebuffers.size(); i++)
    {
        VkImageView imageView = m_pSwapchain->GetImageView(uint32_t(i));
        framebufferParams.pAttachMents = &imageView;
        m_Framebuffers[i] = FFramebuffer::Create(m_pDevice, framebufferParams);
    }
}

void FRenderer::ReleaseFramebuffers()
{
    for (auto& framebuffer : m_Framebuffers)
    {
        delete framebuffer;
    }

    m_Framebuffers.clear();
}
