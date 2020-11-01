#include "Renderer.h"
#include "Model.h"
#include "Camera.h"

#include "Vulkan/VulkanBuffer.h"
#include "Vulkan/VulkanRenderPass.h"
#include "Vulkan/VulkanFramebuffer.h"
#include "Vulkan/VulkanShaderModule.h"
#include "Vulkan/VulkanPipelineState.h"
#include "Vulkan/VulkanCommandBuffer.h"
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
	pipelineParams.pBindingDescriptions 	= &bindingDescription;
	pipelineParams.BindingDescriptionCount 	= 1;
	pipelineParams.pAttributeDescriptions 		= Vertex::GetAttributeDescriptions();
	pipelineParams.AttributeDescriptionCount 	= 3;
	pipelineParams.pVertex 		= pVertex;
	pipelineParams.pFragment 	= pFragment;
	pipelineParams.pRenderPass 	= m_pRenderPass;
	m_PipelineState = m_pContext->CreateGraphicsPipelineState(pipelineParams);

	delete pVertex;
	delete pFragment;
   
	// Framebuffers
	CreateFramebuffers();

	// Commandbuffers
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

	// Allocator for GPU mem
	m_pDeviceAllocator = m_pContext->CreateDeviceAllocator();

	m_pModel = new Model();
	m_pModel->LoadFromFile("res/models/viking_room.obj", m_pContext, m_pDeviceAllocator);
	
	// Camera
	BufferParams camBuffParams;
	camBuffParams.SizeInBytes 		= sizeof(CameraBuffer);
	camBuffParams.MemoryProperties 	= VK_GPU_BUFFER_USAGE;
	camBuffParams.Usage 			= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	
	m_pCameraBuffer = m_pContext->CreateBuffer(camBuffParams, m_pDeviceAllocator);
}

void Renderer::Tick(float dt)
{
	// Update
	m_Camera.Update();
	
	// Draw
	uint32_t frameIndex = m_pContext->GetCurrentBackBufferIndex();
	m_pCurrentCommandBuffer = m_CommandBuffers[frameIndex];

	// Begin Commandbuffer
	m_pCurrentCommandBuffer->Reset();
	m_pCurrentCommandBuffer->Begin();

	// Update camera
	CameraBuffer camBuff;
	camBuff.Projection = m_Camera.GetProjectionMatrix();
	camBuff.View = m_Camera.GetViewMatrix();
	m_pCurrentCommandBuffer->UpdateBuffer(m_pCameraBuffer, 0, sizeof(CameraBuffer), &camBuff);
	
	// Begin renderpass
	VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_pCurrentCommandBuffer->BeginRenderPass(m_pRenderPass, m_Framebuffers[frameIndex], &clearColor, 1);
	
	// Set viewport
	VkExtent2D extent = m_pContext->GetFramebufferExtent();
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

void Renderer::ReleaseFramebuffers()
{
	for (auto& framebuffer : m_Framebuffers)
	{
		delete framebuffer;
	}

	m_Framebuffers.clear();
}
