#include "Renderer.h"
#include "Model.h"
#include "Camera.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/RenderPass.h"
#include "Vulkan/Framebuffer.h"
#include "Vulkan/ShaderModule.h"
#include "Vulkan/PipelineState.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/VulkanDeviceAllocator.h"

Renderer::Renderer()
    : m_pContext(nullptr)
    , m_pRenderPass(nullptr)
    , m_PipelineState(nullptr)
    , m_pCurrentCommandBuffer(nullptr)
    , m_pModel(nullptr)
    , m_pDeviceAllocator(nullptr)
    , m_CommandBuffers()
    , m_Framebuffers()
{
}

void Renderer::Init(VulkanContext* pContext)
{
    // Set context
    m_pContext = pContext;
    
    // PipelineState, RenderPass and Shaders
    ShaderModule* pVertex   = ShaderModule::CreateFromFile(m_pContext, "main", "res/shaders/vertex.spv");
    ShaderModule* pFragment = ShaderModule::CreateFromFile(m_pContext, "main", "res/shaders/fragment.spv");

    RenderPassAttachment attachments[1];
    attachments[0].Format = m_pContext->GetSwapChainFormat();

    RenderPassParams renderPassParams = {};
    renderPassParams.ColorAttachmentCount = 1;
    renderPassParams.pColorAttachments    = attachments;
    m_pRenderPass = RenderPass::Create(m_pContext, renderPassParams);

    VkVertexInputBindingDescription bindingDescription = Vertex::GetBindingDescription();

    GraphicsPipelineStateParams pipelineParams = {};
    pipelineParams.pBindingDescriptions      = &bindingDescription;
    pipelineParams.BindingDescriptionCount   = 1;
    pipelineParams.pAttributeDescriptions    = Vertex::GetAttributeDescriptions();
    pipelineParams.AttributeDescriptionCount = 3;
    pipelineParams.pVertex                   = pVertex;
    pipelineParams.pFragment                 = pFragment;
    pipelineParams.pRenderPass               = m_pRenderPass;
    m_PipelineState = GraphicsPipeline::Create(m_pContext, pipelineParams);

    delete pVertex;
    delete pFragment;
   
    // Framebuffers
    CreateFramebuffers();

    // Commandbuffers
    CommandBufferParams commandBufferParams = {};
    commandBufferParams.Level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferParams.QueueType = ECommandQueueType::Graphics;

    uint32_t imageCount = m_pContext->GetNumBackBuffers();
    m_CommandBuffers.resize(imageCount);
    for (size_t i = 0; i < m_CommandBuffers.size(); i++)
    {
        CommandBuffer* pCommandBuffer = CommandBuffer::Create(m_pContext, commandBufferParams);
        m_CommandBuffers[i] = pCommandBuffer;
    }

    // Allocator for GPU mem
    m_pDeviceAllocator = new VulkanDeviceAllocator(m_pContext->GetDevice(), m_pContext->GetPhysicalDevice());

    m_pModel = new Model();
    m_pModel->LoadFromFile("res/models/viking_room.obj", m_pContext, m_pDeviceAllocator);
    
    // Camera
    BufferParams camBuffParams;
    camBuffParams.SizeInBytes      = sizeof(CameraBuffer);
    camBuffParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    camBuffParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    m_pCameraBuffer = Buffer::Create(m_pContext, camBuffParams, m_pDeviceAllocator);
    
    // Create descriptorpool
    DescriptorPoolParams poolParams;
    poolParams.NumUniformBuffers = 1;
    poolParams.MaxSets           = 1;
    m_pDescriptorPool = DescriptorPool::Create(m_pContext, poolParams);
}

void Renderer::Tick(float deltaTime)
{
    // Update
    VkExtent2D extent = m_pContext->GetFramebufferExtent();
    m_Camera.Update(90.0f, extent.width, extent.height, 0.1f, 100.0f);
    
    // Draw
    uint32_t frameIndex = m_pContext->GetCurrentBackBufferIndex();
    m_pCurrentCommandBuffer = m_CommandBuffers[frameIndex];

    // Begin Commandbuffer
    m_pCurrentCommandBuffer->Reset();
    m_pCurrentCommandBuffer->Begin();

    // Update camera
    CameraBuffer camBuff;
    camBuff.Projection = m_Camera.GetProjectionMatrix();
    camBuff.View       = m_Camera.GetViewMatrix();
    m_pCurrentCommandBuffer->UpdateBuffer(m_pCameraBuffer, 0, sizeof(CameraBuffer), &camBuff);
    
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
    m_pContext->ExecuteGraphics(m_pCurrentCommandBuffer, waitStages);
}

void Renderer::Release()
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

void Renderer::OnWindowResize(uint32_t width, uint32_t height)
{
    ReleaseFramebuffers();
    CreateFramebuffers();
}

void Renderer::CreateFramebuffers()
{
    uint32_t imageCount = m_pContext->GetNumBackBuffers();
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
        m_Framebuffers[i] = Framebuffer::Create(m_pContext, framebufferParams);
    }
}

void Renderer::ReleaseFramebuffers()
{
    for (auto& framebuffer : m_Framebuffers)
    {
        delete framebuffer;
    }

    m_Framebuffers.clear();
}
