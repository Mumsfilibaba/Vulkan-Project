#include "RayTracer.h"
#include "Application.h"
#include "MathHelper.h"
#include "GUI.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/Framebuffer.h"
#include "Vulkan/ShaderModule.h"
#include "Vulkan/PipelineState.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/DeviceMemoryAllocator.h"
#include "Vulkan/DescriptorSet.h"
#include "Vulkan/Query.h"
#include "Vulkan/DescriptorSetLayout.h"
#include "Vulkan/PipelineLayout.h"
#include "Vulkan/Swapchain.h"
#include "Vulkan/Texture.h"
#include "Vulkan/TextureView.h"

RayTracer::RayTracer()
    : m_pDevice(nullptr)
    , m_Pipeline(nullptr)
    , m_pDeviceAllocator(nullptr)
    , m_CommandBuffers()
    , m_pSceneTexture(nullptr)
    , m_pSceneTextureView(nullptr)
    , m_pSceneTextureDescriptorSet(nullptr)
{
}

void RayTracer::Init(Device* pDevice, Swapchain* pSwapchain)
{
    // Set device
    m_pDevice    = pDevice;
    m_pSwapchain = pSwapchain;

    // Create DescriptorSetLayout
    constexpr uint32_t numBindings = 3;
    VkDescriptorSetLayoutBinding bindings[numBindings];
    bindings[0].binding            = 0;
    bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[0].descriptorCount    = 1;
    bindings[0].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[0].pImmutableSamplers = nullptr;
    
    bindings[1].binding            = 1;
    bindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[1].descriptorCount    = 1;
    bindings[1].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[1].pImmutableSamplers = nullptr;
    
    bindings[2].binding            = 2;
    bindings[2].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[2].descriptorCount    = 1;
    bindings[2].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[2].pImmutableSamplers = nullptr;

    DescriptorSetLayoutParams descriptorSetLayoutParams;
    descriptorSetLayoutParams.pBindings   = bindings;
    descriptorSetLayoutParams.numBindings = numBindings;

    m_pDescriptorSetLayout = DescriptorSetLayout::Create(m_pDevice, descriptorSetLayoutParams);
    assert(m_pDescriptorSetLayout != nullptr);

    // Create PipelineLayout
    PipelineLayoutParams pipelineLayoutParams;
    pipelineLayoutParams.ppLayouts  = &m_pDescriptorSetLayout;
    pipelineLayoutParams.numLayouts = 1;

    m_pPipelineLayout = PipelineLayout::Create(m_pDevice, pipelineLayoutParams);
    assert(m_pPipelineLayout != nullptr);

    // Create shader and pipeline
    ShaderModule* pComputeShader = ShaderModule::CreateFromFile(m_pDevice, "main", "res/shaders/raytracer.spv");

    ComputePipelineStateParams pipelineParams = {};
    pipelineParams.pShader         = pComputeShader;
    pipelineParams.pPipelineLayout = m_pPipelineLayout;
    
    m_Pipeline = ComputePipeline::Create(m_pDevice, pipelineParams);
    assert(m_Pipeline != nullptr);

    delete pComputeShader;
   
    // Create DescriptorPool
    DescriptorPoolParams poolParams;
    poolParams.NumUniformBuffers = 6;
    poolParams.NumStorageImages  = 3;
    poolParams.MaxSets           = 3;
    m_pDescriptorPool = DescriptorPool::Create(m_pDevice, poolParams);
    assert(m_pDescriptorPool != nullptr);

    // Camera
    BufferParams camBuffParams;
    camBuffParams.Size             = sizeof(CameraBuffer);
    camBuffParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    camBuffParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pCameraBuffer = Buffer::Create(m_pDevice, camBuffParams, m_pDeviceAllocator);
    assert(m_pCameraBuffer != nullptr);

    // Random
    BufferParams randBuffParams;
    randBuffParams.Size             = sizeof(RandomBuffer);
    randBuffParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    randBuffParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pRandomBuffer = Buffer::Create(m_pDevice, randBuffParams, m_pDeviceAllocator);
    assert(m_pRandomBuffer != nullptr);
  
    // Create the scene texture
    m_ViewportWidth  = 0;
    m_ViewportHeight = 0;
    CreateOrResizeSceneTexture(1280, 720);

    // CommandBuffers
    CommandBufferParams commandBufferParams = {};
    commandBufferParams.Level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferParams.QueueType = ECommandQueueType::Graphics;

    uint32_t imageCount = m_pSwapchain->GetNumBackBuffers();
    m_CommandBuffers.resize(imageCount);
    for (size_t i = 0; i < m_CommandBuffers.size(); i++)
    {
        CommandBuffer* pCommandBuffer = CommandBuffer::Create(m_pDevice, commandBufferParams);
        m_CommandBuffers[i] = pCommandBuffer;
    }

    // Timestamp queries
    QueryParams queryParams;
    queryParams.queryType  = VK_QUERY_TYPE_TIMESTAMP;
    queryParams.queryCount = 2;
    
    m_TimestampQueries.resize(imageCount);
    for (size_t i = 0; i < m_TimestampQueries.size(); i++)
    {
        Query* pQuery = Query::Create(m_pDevice, queryParams);
        pQuery->Reset();
        
        m_TimestampQueries[i] = pQuery;
    }
    
    // Allocator for GPU mem
    m_pDeviceAllocator = new DeviceMemoryAllocator(m_pDevice->GetDevice(), m_pDevice->GetPhysicalDevice());
}

void RayTracer::Tick(float deltaTime)
{
    constexpr float CameraSpeed = 1.5f;
    
    // Update scene image
    CreateOrResizeSceneTexture(m_ViewportWidth, m_ViewportHeight);
    
    glm::vec3 translation(0.0f);
    if (glfwGetKey(Application::GetWindow(), GLFW_KEY_W) == GLFW_PRESS)
    {
        translation.z = CameraSpeed * deltaTime;
    }
    else if (glfwGetKey(Application::GetWindow(), GLFW_KEY_S) == GLFW_PRESS)
    {
        translation.z = -CameraSpeed * deltaTime;
    }
    
    if (glfwGetKey(Application::GetWindow(), GLFW_KEY_A) == GLFW_PRESS)
    {
        translation.x = CameraSpeed * deltaTime;
    }
    else if (glfwGetKey(Application::GetWindow(), GLFW_KEY_D) == GLFW_PRESS)
    {
        translation.x = -CameraSpeed * deltaTime;
    }
    
    m_Camera.Move(translation);

    constexpr float CameraRotationSpeed = glm::pi<float>() / 2;
    
    glm::vec3 rotation(0.0f);
    if (glfwGetKey(Application::GetWindow(), GLFW_KEY_LEFT) == GLFW_PRESS)
    {
        rotation.y = -CameraRotationSpeed * deltaTime;
    }
    else if (glfwGetKey(Application::GetWindow(), GLFW_KEY_RIGHT) == GLFW_PRESS)
    {
        rotation.y = CameraRotationSpeed * deltaTime;
    }
    
    if (glfwGetKey(Application::GetWindow(), GLFW_KEY_UP) == GLFW_PRESS)
    {
        rotation.x = -CameraRotationSpeed * deltaTime;
    }
    else if (glfwGetKey(Application::GetWindow(), GLFW_KEY_DOWN) == GLFW_PRESS)
    {
        rotation.x = CameraRotationSpeed * deltaTime;
    }
    
    m_Camera.Rotate(rotation);
    
    // Update
    m_Camera.Update(90.0f, m_pSceneTexture->GetWidth(), m_pSceneTexture->GetHeight(), 0.1f, 100.0f);
    
    // Draw
    uint32_t frameIndex = m_pSwapchain->GetCurrentBackBufferIndex();
    Query*         pCurrentTimestampQuery = m_TimestampQueries[frameIndex];
    CommandBuffer* pCurrentCommandBuffer  = m_CommandBuffers[frameIndex];
    
    // Begin CommandBuffer
    pCurrentCommandBuffer->Reset();
    
    constexpr uint32_t timestampCount = 2;
    uint64_t timestamps[timestampCount];
    ZERO_MEMORY(timestamps, sizeof(uint64_t) * timestampCount);
    
    pCurrentTimestampQuery->GetData(0, 2, sizeof(uint64_t) * timestampCount, &timestamps, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
    pCurrentTimestampQuery->Reset();
    
    const double timestampPeriod = double(m_pDevice->GetTimestampPeriod());
    const double gpuTiming       = (double(timestamps[1]) - double(timestamps[0])) * timestampPeriod;
    const double gpuTimingMS     = gpuTiming / 1000000.0;
    m_LastGPUTime = static_cast<float>(gpuTimingMS);
    
    pCurrentCommandBuffer->Begin();
    pCurrentCommandBuffer->WriteTimestamp(pCurrentTimestampQuery, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
    
    pCurrentCommandBuffer->TransitionImage(m_pSceneTexture->GetImage(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

    // Update camera
    CameraBuffer camBuff;
    camBuff.Projection = m_Camera.GetProjectionMatrix();
    camBuff.View       = m_Camera.GetViewMatrix();
    camBuff.Position   = glm::vec4(m_Camera.GetPosition(), 0.0f);
    camBuff.Forward    = glm::vec4(m_Camera.GetForward(), 0.0f);
    pCurrentCommandBuffer->UpdateBuffer(m_pCameraBuffer, 0, sizeof(CameraBuffer), &camBuff);
    
    // Update random
    static uint32_t randFrameIndex = 0;
    randFrameIndex++;
    
    if (randFrameIndex >= 16)
    {
        randFrameIndex = 0;
    }
    
    RandomBuffer randBuff;
    randBuff.FrameIndex = randFrameIndex;
    pCurrentCommandBuffer->UpdateBuffer(m_pRandomBuffer, 0, sizeof(RandomBuffer), &randBuff);
    
    // Bind pipeline and descriptorSet
    pCurrentCommandBuffer->BindComputePipelineState(m_Pipeline);
    pCurrentCommandBuffer->BindComputeDescriptorSet(m_pPipelineLayout, m_pDescriptorSet);
    
    // Dispatch
    const uint32_t Threads = 16;
    VkExtent2D dispatchSize = { Math::AlignUp(m_pSceneTexture->GetWidth(), Threads) / Threads, Math::AlignUp(m_pSceneTexture->GetHeight(), Threads) / Threads };
    pCurrentCommandBuffer->Dispatch(dispatchSize.width, dispatchSize.height, 1);
    
    pCurrentCommandBuffer->TransitionImage(m_pSceneTexture->GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    pCurrentCommandBuffer->WriteTimestamp(pCurrentTimestampQuery, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1);
    pCurrentCommandBuffer->End();

    m_pDevice->ExecuteGraphics(pCurrentCommandBuffer, nullptr, nullptr);
}

void RayTracer::OnRenderUI()
{
    static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;
    
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;

    const ImGuiViewport* pMainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(pMainViewport->WorkPos);
    ImGui::SetNextWindowSize(pMainViewport->WorkSize);
    ImGui::SetNextWindowViewport(pMainViewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
    {
        windowFlags |= ImGuiWindowFlags_NoBackground;
    }

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", nullptr, windowFlags);
    ImGui::PopStyleVar();

    ImGui::PopStyleVar(2);

    // Submit the DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspaceID = ImGui::GetID("PathTracerDockspace");
        ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), dockspaceFlags);
    }

    ImGui::End();
    
    ImGui::Begin("Scene");
    ImGui::Text("GPU Time %.4f", m_LastGPUTime);
    
    
    ImGui::End();
    
    ImGui::Begin("Viewport");

    m_ViewportWidth  = ImGui::GetContentRegionAvail().x;
    m_ViewportHeight = ImGui::GetContentRegionAvail().y;

    if (m_pSceneTexture)
    {
        ImGui::Image(m_pSceneTextureDescriptorSet, { (float)m_pSceneTexture->GetWidth(), (float)m_pSceneTexture->GetHeight() });
    }
    
    ImGui::End();
}

void RayTracer::Release()
{
    for (auto& commandBuffer : m_CommandBuffers)
    {
        SAFE_DELETE(commandBuffer);
    }
    
    m_CommandBuffers.clear();
    
    for (auto& query : m_TimestampQueries)
    {
        SAFE_DELETE(query);
    }
    
    m_TimestampQueries.clear();

    SAFE_DELETE(m_pCameraBuffer);
    SAFE_DELETE(m_pRandomBuffer);
    
    ReleaseDescriptorSet();

    SAFE_DELETE(m_pDescriptorPool);
    SAFE_DELETE(m_Pipeline);
    SAFE_DELETE(m_pPipelineLayout);
    SAFE_DELETE(m_pDescriptorSetLayout);
    SAFE_DELETE(m_pDeviceAllocator);
    SAFE_DELETE(m_pSceneTexture);
    SAFE_DELETE(m_pSceneTextureView);
    SAFE_DELETE(m_pSceneTextureDescriptorSet);
}

void RayTracer::OnWindowResize(uint32_t width, uint32_t height)
{
}

void RayTracer::CreateOrResizeSceneTexture(uint32_t width, uint32_t height)
{
    if (m_pSceneTexture)
    {
        if ((m_pSceneTexture->GetWidth() == width && m_pSceneTexture->GetHeight() == height) || width == 0 || height == 0)
        {
            return;
        }
        
        m_pDevice->WaitForIdle();
        
        SAFE_DELETE(m_pSceneTexture);
        SAFE_DELETE(m_pSceneTextureView);
        SAFE_DELETE(m_pSceneTextureDescriptorSet);
        ReleaseDescriptorSet();
    }
    
    // Create texture for the viewport
    TextureParams textureParams = {};
    textureParams.Format    = VK_FORMAT_R32G32B32A32_SFLOAT;
    textureParams.ImageType = VK_IMAGE_TYPE_2D;
    textureParams.Width     = m_ViewportWidth  = width;
    textureParams.Height    = m_ViewportHeight = height;
    textureParams.Usage     = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    
    m_pSceneTexture = Texture::Create(m_pDevice, textureParams);
    assert(m_pSceneTexture != nullptr);
    
    TextureViewParams textureViewParams = {};
    textureViewParams.pTexture = m_pSceneTexture;
    
    m_pSceneTextureView = TextureView::Create(m_pDevice, textureViewParams);
    assert(m_pSceneTextureView != nullptr);
    
    // UI DescriptorSet
    m_pSceneTextureDescriptorSet = GUI::AllocateTextureID(m_pSceneTextureView);
    assert(m_pSceneTextureDescriptorSet != nullptr);
    
    // Descriptor set for when tracing
    CreateDescriptorSet();
}

void RayTracer::CreateDescriptorSet()
{
    m_pDescriptorSet = DescriptorSet::Create(m_pDevice, m_pDescriptorPool, m_pDescriptorSetLayout);
    assert(m_pDescriptorSet != nullptr);
        
    m_pDescriptorSet->BindStorageImage(m_pSceneTextureView->GetImageView(), 0);
    m_pDescriptorSet->BindUniformBuffer(m_pCameraBuffer->GetBuffer(), 1);
    m_pDescriptorSet->BindUniformBuffer(m_pRandomBuffer->GetBuffer(), 2);
}

void RayTracer::ReleaseDescriptorSet()
{
    SAFE_DELETE(m_pDescriptorSet);
}
