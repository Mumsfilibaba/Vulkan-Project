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

    FRenderPassAttachment Attachments[1];
    Attachments[0].Format = m_pSwapchain->GetFormat();

    FRenderPassParams RenderPassParams = {};
    RenderPassParams.ColorAttachmentCount = 1;
    RenderPassParams.pColorAttachments    = Attachments;
    m_pRenderPass = FRenderPass::Create(m_pDevice, RenderPassParams);

    FGraphicsPipelineStateParams PipelineParams = {};
    PipelineParams.pBindingDescriptions      = FVertex::GetBindingDescription();
    PipelineParams.BindingDescriptionCount   = 1;
    PipelineParams.pAttributeDescriptions    = FVertex::GetAttributeDescriptions();
    PipelineParams.AttributeDescriptionCount = 3;
    PipelineParams.pVertexShader             = pVertex;
    PipelineParams.pFragmentShader           = pFragment;
    PipelineParams.pRenderPass               = m_pRenderPass;
    m_PipelineState = FGraphicsPipeline::Create(m_pDevice, PipelineParams);

    delete pVertex;
    delete pFragment;
   
    // Framebuffers
    CreateFramebuffers();

    // CommandBuffers
    FCommandBufferParams CommandBufferParams = {};
    CommandBufferParams.Level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferParams.QueueType = ECommandQueueType::Graphics;

    uint32_t ImageCount = m_pSwapchain->GetNumBackBuffers();
    m_CommandBuffers.resize(ImageCount);
    for (size_t i = 0; i < m_CommandBuffers.size(); i++)
    {
        FCommandBuffer* pCommandBuffer = FCommandBuffer::Create(m_pDevice, CommandBufferParams);
        m_CommandBuffers[i] = pCommandBuffer;
    }

    // Allocator for GPU mem
    m_pDeviceAllocator = new FDeviceMemoryAllocator(pDevice);

    m_pModel = new FModel();
    m_pModel->LoadFromFile("res/models/viking_room.obj", m_pDevice, m_pDeviceAllocator);
    
    // Camera
    FBufferParams CameraBufferParams;
    CameraBufferParams.Size      = sizeof(FCameraBuffer);
    CameraBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    CameraBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    m_pCameraBuffer = FBuffer::Create(m_pDevice, CameraBufferParams, m_pDeviceAllocator);
    
    // Create descriptorpool
    FDescriptorPoolParams DescriptorPoolParams;
    DescriptorPoolParams.NumUniformBuffers = 1;
    DescriptorPoolParams.MaxSets           = 1;
    m_pDescriptorPool = FDescriptorPool::Create(m_pDevice, DescriptorPoolParams);
}

void FRenderer::Tick(float DeltaTime)
{
    // Update
    VkExtent2D Extent = m_pSwapchain->GetExtent();
    m_Camera.Update(90.0f, Extent.width, Extent.height, 0.1f, 100.0f);
    
    // Draw
    uint32_t FrameIndex = m_pSwapchain->GetCurrentBackBufferIndex();
    m_pCurrentCommandBuffer = m_CommandBuffers[FrameIndex];

    // Begin CommandBuffer
    m_pCurrentCommandBuffer->Reset();
    m_pCurrentCommandBuffer->Begin();

    // Update camera
    FCameraBuffer CameraBuffer;
    CameraBuffer.Projection = m_Camera.GetProjectionMatrix();
    CameraBuffer.View       = m_Camera.GetViewMatrix();
    m_pCurrentCommandBuffer->UpdateBuffer(m_pCameraBuffer, 0, sizeof(FCameraBuffer), &CameraBuffer);
    
    // Begin renderpass
    VkClearValue ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_pCurrentCommandBuffer->BeginRenderPass(m_pRenderPass, m_Framebuffers[FrameIndex], &ClearColor, 1);
    
    // Set viewport
    VkViewport Ciewport = { 0.0f, 0.0f, float(Extent.width), float(Extent.height), 0.0f, 1.0f };
    m_pCurrentCommandBuffer->SetViewport(Ciewport);
    VkRect2D Scissor = { { 0, 0 }, Extent };
    m_pCurrentCommandBuffer->SetScissorRect(Scissor);
    
    // Bind pipeline
    m_pCurrentCommandBuffer->BindGraphicsPipelineState(m_PipelineState);

    // Draw
    m_pCurrentCommandBuffer->BindVertexBuffer(m_pModel->GetVertexBuffer(), 0, 0);
    m_pCurrentCommandBuffer->BindIndexBuffer(m_pModel->GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);
    m_pCurrentCommandBuffer->DrawIndexInstanced(m_pModel->GetIndexCount(), 1, 0, 0, 0);
    
    // End renderpass
    m_pCurrentCommandBuffer->EndRenderPass();
    m_pCurrentCommandBuffer->End();

    VkPipelineStageFlags WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    m_pDevice->ExecuteGraphics(m_pCurrentCommandBuffer, m_pSwapchain, WaitStages);
}

void FRenderer::Release()
{
    delete m_pModel;

    for (auto& CommandBuffer : m_CommandBuffers)
    {
        delete CommandBuffer;
    }
    
    m_CommandBuffers.clear();

    ReleaseFramebuffers();

    delete m_pRenderPass;
    delete m_PipelineState;

    delete m_pDeviceAllocator;    
}

void FRenderer::OnWindowResize(uint32_t Width, uint32_t Height)
{
    ReleaseFramebuffers();
    CreateFramebuffers();
}

void FRenderer::CreateFramebuffers()
{
    uint32_t ImageCount = m_pSwapchain->GetNumBackBuffers();
    m_Framebuffers.resize(ImageCount);

    VkExtent2D Extent = m_pSwapchain->GetExtent();
    
    FFramebufferParams FramebufferParams = {};
    FramebufferParams.AttachmentCount   = 1;
    FramebufferParams.Width             = Extent.width;
    FramebufferParams.Height            = Extent.height;
    FramebufferParams.pRenderPass       = m_pRenderPass;

    for (size_t i = 0; i < m_Framebuffers.size(); i++)
    {
        VkImageView ImageView = m_pSwapchain->GetImageView(uint32_t(i));
        FramebufferParams.pAttachMents = &ImageView;
        m_Framebuffers[i] = FFramebuffer::Create(m_pDevice, FramebufferParams);
    }
}

void FRenderer::ReleaseFramebuffers()
{
    for (auto& Framebuffer : m_Framebuffers)
    {
        delete Framebuffer;
    }

    m_Framebuffers.clear();
}
