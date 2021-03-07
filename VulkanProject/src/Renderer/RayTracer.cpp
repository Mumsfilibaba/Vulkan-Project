#include "RayTracer.h"

#include "MathHelper.h"

#include "Vulkan/Buffer.h"
#include "Vulkan/Framebuffer.h"
#include "Vulkan/ShaderModule.h"
#include "Vulkan/PipelineState.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/VulkanDeviceAllocator.h"
#include "Vulkan/DescriptorSet.h"

RayTracer::RayTracer()
	: m_pContext(nullptr)
	, m_Pipeline(nullptr)
	, m_pCurrentCommandBuffer(nullptr)
	, m_pDeviceAllocator(nullptr)
	, m_CommandBuffers()
{
}

void RayTracer::Init(VulkanContext* pContext)
{
	// Set context
	m_pContext = pContext;
	
	ShaderModule* pCompute = ShaderModule::CreateFromFile(m_pContext, "main", "res/shaders/raytracer.spv");

	ComputePipelineStateParams pipelineParams = {};
	pipelineParams.pShader = pCompute;
	m_Pipeline = ComputePipeline::Create(m_pContext, pipelineParams);

	delete pCompute;
   
	// Create descriptorpool
	DescriptorPoolParams poolParams;
	poolParams.NumUniformBuffers = 3;
	poolParams.NumStorageImages  = 3;
	poolParams.MaxSets			 = 3;
	m_pDescriptorPool = DescriptorPool::Create(m_pContext, poolParams);

	// DescriptorSets
	CreateDescriptorSets();

	// Commandbuffers
	CommandBufferParams commandBufferParams = {};
	commandBufferParams.Level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferParams.QueueType = ECommandQueueType::Graphics; // More like direct

	uint32_t imageCount = m_pContext->GetNumBackBuffers();
	m_CommandBuffers.resize(imageCount);
	for (size_t i = 0; i < m_CommandBuffers.size(); i++)
	{
		CommandBuffer* pCommandBuffer = CommandBuffer::Create(m_pContext, commandBufferParams);
		m_CommandBuffers[i] = pCommandBuffer;
	}

	// Allocator for GPU mem
	m_pDeviceAllocator = new VulkanDeviceAllocator(m_pContext->GetDevice(), m_pContext->GetPhysicalDevice());
	
	// Camera
	BufferParams camBuffParams;
	camBuffParams.SizeInBytes 		= sizeof(CameraBuffer);
	camBuffParams.MemoryProperties 	= VK_GPU_BUFFER_USAGE;
	camBuffParams.Usage 			= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	m_pCameraBuffer = Buffer::Create(m_pContext, camBuffParams, m_pDeviceAllocator);
	
}

void RayTracer::Tick(float dt)
{
	// Update
	m_Camera.Update();
	
	// Draw
	uint32_t frameIndex = m_pContext->GetCurrentBackBufferIndex();
	m_pCurrentCommandBuffer = m_CommandBuffers[frameIndex];

	// Begin Commandbuffer
	m_pCurrentCommandBuffer->Reset();
	m_pCurrentCommandBuffer->Begin();
	
	m_pCurrentCommandBuffer->TransitionImage(m_pContext->GetSwapChainImage(frameIndex), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	// Update camera
	CameraBuffer camBuff;
	camBuff.Projection = m_Camera.GetProjectionMatrix();
	camBuff.View       = m_Camera.GetViewMatrix();
	m_pCurrentCommandBuffer->UpdateBuffer(m_pCameraBuffer, 0, sizeof(CameraBuffer), &camBuff);
	
	// Bind pipeline and descriptorSet
	m_pCurrentCommandBuffer->BindComputePipelineState(m_Pipeline);
	m_pCurrentCommandBuffer->BindComputeDescriptorSet(m_Pipeline, m_DescriptorSets[frameIndex]);
	
	// Dispatch
	VkExtent2D extent = m_pContext->GetFramebufferExtent();
	VkExtent2D dispatchSize = { Math::AlignUp(extent.width, 16) / 16, Math::AlignUp(extent.height, 16) / 16 };
	m_pCurrentCommandBuffer->Dispatch(dispatchSize.width, dispatchSize.height, 1);
	
	m_pCurrentCommandBuffer->TransitionImage(m_pContext->GetSwapChainImage(frameIndex), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	
	m_pCurrentCommandBuffer->End();

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
	m_pContext->ExecuteGraphics(m_pCurrentCommandBuffer, waitStages);
}

void RayTracer::Release()
{
	for (auto& commandBuffer : m_CommandBuffers)
	{
		delete commandBuffer;
	}
	m_CommandBuffers.clear();

	delete m_pCameraBuffer;

	ReleaseDescriptorSets();

	delete m_pDescriptorPool;
	delete m_Pipeline;
	delete m_pDeviceAllocator;
}

void RayTracer::OnWindowResize(uint32_t width, uint32_t height)
{
	(void)width;
	(void)height;
	
	ReleaseDescriptorSets();
	CreateDescriptorSets();
}

void RayTracer::CreateDescriptorSets()
{
	uint32_t numBackBuffers = m_pContext->GetNumBackBuffers();
	m_DescriptorSets.resize(numBackBuffers);
	
	for (uint32_t i = 0; i < numBackBuffers; i++)
	{
		m_DescriptorSets[i] = DescriptorSet::Create(m_pContext, m_pDescriptorPool, m_Pipeline);
		assert(m_DescriptorSets[i] != nullptr);
		
		m_DescriptorSets[i]->BindStorageImage(m_pContext->GetSwapChainImageView(i), 0);
	}
}

void RayTracer::ReleaseDescriptorSets()
{
	for (auto& descriptorSet : m_DescriptorSets)
	{
		delete descriptorSet;
	}

	m_DescriptorSets.clear();
}
