#include "RayTracer.h"
#include "Application.h"
#include "Input.h"
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
#include "Vulkan/Helpers.h"

RayTracer::RayTracer()
    : m_pDevice(nullptr)
    , m_pPipeline(nullptr)
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
    constexpr uint32_t numBindings = 7;
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

    bindings[3].binding            = 3;
    bindings[3].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[3].descriptorCount    = 1;
    bindings[3].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[3].pImmutableSamplers = nullptr;

    bindings[4].binding            = 4;
    bindings[4].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[4].descriptorCount    = 1;
    bindings[4].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[4].pImmutableSamplers = nullptr;

    bindings[5].binding            = 5;
    bindings[5].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[5].descriptorCount    = 1;
    bindings[5].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[5].pImmutableSamplers = nullptr;

    bindings[6].binding            = 6;
    bindings[6].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[6].descriptorCount    = 1;
    bindings[6].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[6].pImmutableSamplers = nullptr;

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
    
    m_pPipeline = ComputePipeline::Create(m_pDevice, pipelineParams);
    assert(m_pPipeline != nullptr);

    delete pComputeShader;
   
    // Create DescriptorPool
    DescriptorPoolParams poolParams;
    poolParams.NumUniformBuffers = 3;
    poolParams.NumStorageImages  = 1;
    poolParams.NumStorageBuffers = 3;
    poolParams.MaxSets           = 1;
    m_pDescriptorPool = DescriptorPool::Create(m_pDevice, poolParams);
    assert(m_pDescriptorPool != nullptr);

    // Camera
    BufferParams cameraBufferParams;
    cameraBufferParams.Size             = sizeof(CameraBuffer);
    cameraBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    cameraBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pCameraBuffer = Buffer::Create(m_pDevice, cameraBufferParams, m_pDeviceAllocator);
    assert(m_pCameraBuffer != nullptr);

    // Random
    BufferParams randomBufferParams;
    randomBufferParams.Size             = sizeof(RandomBuffer);
    randomBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    randomBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pRandomBuffer = Buffer::Create(m_pDevice, randomBufferParams, m_pDeviceAllocator);
    assert(m_pRandomBuffer != nullptr);

    // SceneBuffer
    BufferParams sceneBufferParams;
    sceneBufferParams.Size             = sizeof(SceneBuffer);
    sceneBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    sceneBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pSceneBuffer = Buffer::Create(m_pDevice, sceneBufferParams, m_pDeviceAllocator);
    assert(m_pSceneBuffer != nullptr);
  
    // SphereBuffer
    BufferParams sphereBufferParams;
    sphereBufferParams.Size             = sizeof(Sphere) * MAX_SPHERES;
    sphereBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    sphereBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_Spheres.reserve(MAX_SPHERES);
    m_Spheres.push_back({ glm::vec3(1.0f, 0.5f, 1.0f), 0.45f, 0 });
    m_Spheres.push_back({ glm::vec3(0.0f, 0.5f, 1.0f), 0.45f, 0 });
    m_Spheres.push_back({ glm::vec3(-1.0f, 0.5f, 1.0f), 0.45f, 0 });
    m_Spheres.push_back({ glm::vec3(1.0f, 0.5f, 0.0f), 0.45f, 1 });
    m_Spheres.push_back({ glm::vec3(0.0f, 0.5f, 0.0f), 0.45f, 1 });
    m_Spheres.push_back({ glm::vec3(-1.0f, 0.5f, 0.0f), 0.45f, 1 });
    m_Spheres.push_back({ glm::vec3(1.0f, 0.5f, -1.0f), 0.45f, 2 });
    m_Spheres.push_back({ glm::vec3(0.0f, 0.5f, -1.0f), 0.45f, 2 });
    m_Spheres.push_back({ glm::vec3(-1.0f, 0.5f, -1.0f), 0.45f, 2 });

    m_pSphereBuffer = Buffer::Create(m_pDevice, sphereBufferParams, m_pDeviceAllocator);
    assert(m_pSphereBuffer != nullptr);

    // PlaneBuffer
    BufferParams planeBufferParams;
    planeBufferParams.Size             = sizeof(Plane) * MAX_PLANES;
    planeBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    planeBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_Planes.reserve(MAX_PLANES);
    m_Planes.push_back({ glm::vec3(0.0f, 1.0f, 0.0),  0.0f, 3 });

    m_pPlaneBuffer = Buffer::Create(m_pDevice, planeBufferParams, m_pDeviceAllocator);
    assert(m_pPlaneBuffer != nullptr);

    // MaterialBuffer
    BufferParams materialBufferParams;
    materialBufferParams.Size             = sizeof(Material) * MAX_MATERIALS;
    materialBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    materialBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_Materials.reserve(MAX_MATERIALS);
    m_Materials.push_back({ glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) });
    m_Materials.push_back({ glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) });
    m_Materials.push_back({ glm::vec4(0.0f, 0.0f, 1.0f, 1.0f) });
    m_Materials.push_back({ glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) });

    m_pMaterialBuffer = Buffer::Create(m_pDevice, materialBufferParams, m_pDeviceAllocator);
    assert(m_pMaterialBuffer != nullptr);

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
    
    // Allocator for GPU memory
    m_pDeviceAllocator = new DeviceMemoryAllocator(m_pDevice->GetDevice(), m_pDevice->GetPhysicalDevice());
}

void RayTracer::Tick(float deltaTime)
{
    constexpr float CameraSpeed = 1.5f;
    m_LastCPUTime = deltaTime * 1000.0f; // deltaTime is in seconds
    
    // Update scene image
    CreateOrResizeSceneTexture(m_ViewportWidth, m_ViewportHeight);
    
    glm::vec3 translation(0.0f);
    if (Input::IsKeyDown(GLFW_KEY_W))
    {
        translation.z = CameraSpeed * deltaTime;
    }
    else if (Input::IsKeyDown(GLFW_KEY_S))
    {
        translation.z = -CameraSpeed * deltaTime;
    }
    
    if (Input::IsKeyDown(GLFW_KEY_A))
    {
        translation.x = CameraSpeed * deltaTime;
    }
    else if (Input::IsKeyDown(GLFW_KEY_D))
    {
        translation.x = -CameraSpeed * deltaTime;
    }
    
    m_Camera.Move(translation);

    constexpr float CameraRotationSpeed = glm::pi<float>() / 2;
    
    glm::vec3 rotation(0.0f);
    if (Input::IsKeyDown(GLFW_KEY_LEFT))
    {
        rotation.y = -CameraRotationSpeed * deltaTime;
    }
    else if (Input::IsKeyDown(GLFW_KEY_RIGHT))
    {
        rotation.y = CameraRotationSpeed * deltaTime;
    }
    
    if (Input::IsKeyDown(GLFW_KEY_UP))
    {
        rotation.x = -CameraRotationSpeed * deltaTime;
    }
    else if (Input::IsKeyDown(GLFW_KEY_DOWN))
    {
        rotation.x = CameraRotationSpeed * deltaTime;
    }
    
    if (Input::IsKeyDown(GLFW_KEY_R))
    {
        ReloadShader();
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

    // Update CameraBuffer
    CameraBuffer cameraBuffer = {};
    cameraBuffer.Projection = m_Camera.GetProjectionMatrix();
    cameraBuffer.View       = m_Camera.GetViewMatrix();
    cameraBuffer.Position   = glm::vec4(m_Camera.GetPosition(), 0.0f);
    cameraBuffer.Forward    = glm::vec4(m_Camera.GetForward(), 0.0f);
    pCurrentCommandBuffer->UpdateBuffer(m_pCameraBuffer, 0, sizeof(CameraBuffer), &cameraBuffer);
    
    // Update RandomBuffer
    static uint32_t randFrameIndex = 0;
    randFrameIndex++;
    
    if (randFrameIndex >= 16)
    {
        randFrameIndex = 0;
    }
    
    RandomBuffer randomBuffer = {};
    randomBuffer.FrameIndex = randFrameIndex;
    pCurrentCommandBuffer->UpdateBuffer(m_pRandomBuffer, 0, sizeof(RandomBuffer), &randomBuffer);

    // Update Scene
    SceneBuffer sceneBuffer = {};
    sceneBuffer.LightDir     = glm::normalize(glm::vec4(0.0f, -1.0f, 0.0f, 0.0f));
    sceneBuffer.NumSpheres   = m_Spheres.size();
    sceneBuffer.NumPlanes    = m_Planes.size();
    sceneBuffer.NumMaterials = m_Materials.size();

    pCurrentCommandBuffer->UpdateBuffer(m_pSceneBuffer, 0, sizeof(SceneBuffer), &sceneBuffer);
    pCurrentCommandBuffer->UpdateBuffer(m_pSphereBuffer, 0, sizeof(Sphere) * m_Spheres.size(), m_Spheres.data());
    pCurrentCommandBuffer->UpdateBuffer(m_pPlaneBuffer, 0, sizeof(Plane) * m_Planes.size(), m_Planes.data());
    pCurrentCommandBuffer->UpdateBuffer(m_pMaterialBuffer, 0, sizeof(Material) * m_Materials.size(), m_Materials.data());

    // Bind pipeline and descriptorSet
    pCurrentCommandBuffer->BindComputePipelineState(m_pPipeline.load());
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
    ImGui::Text("CPU Time %.4f", m_LastCPUTime);
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
    SAFE_DELETE(m_pSceneBuffer);
    SAFE_DELETE(m_pSphereBuffer);
    SAFE_DELETE(m_pPlaneBuffer);
    SAFE_DELETE(m_pMaterialBuffer);

    ReleaseDescriptorSet();

    SAFE_DELETE(m_pDescriptorPool);
    SAFE_DELETE(m_pPipeline);
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
    textureParams.Format        = VK_FORMAT_R32G32B32A32_SFLOAT;
    textureParams.ImageType     = VK_IMAGE_TYPE_2D;
    textureParams.Width         = m_ViewportWidth  = width;
    textureParams.Height        = m_ViewportHeight = height;
    textureParams.Usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    textureParams.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    m_pSceneTexture = Texture::Create(m_pDevice, textureParams);
    assert(m_pSceneTexture != nullptr);
    SetDebugName(m_pDevice->GetDevice(), "SceneTexture", reinterpret_cast<uint64_t>(m_pSceneTexture->GetImage()), VK_OBJECT_TYPE_IMAGE);

    TextureViewParams textureViewParams = {};
    textureViewParams.pTexture = m_pSceneTexture;
    
    m_pSceneTextureView = TextureView::Create(m_pDevice, textureViewParams);
    assert(m_pSceneTextureView != nullptr);
    SetDebugName(m_pDevice->GetDevice(), "SceneTextureView", reinterpret_cast<uint64_t>(m_pSceneTextureView->GetImageView()), VK_OBJECT_TYPE_IMAGE_VIEW);
    
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
    m_pDescriptorSet->BindUniformBuffer(m_pSceneBuffer->GetBuffer(), 3);
    m_pDescriptorSet->BindStorageBuffer(m_pSphereBuffer->GetBuffer(), 4);
    m_pDescriptorSet->BindStorageBuffer(m_pPlaneBuffer->GetBuffer(), 5);
    m_pDescriptorSet->BindStorageBuffer(m_pMaterialBuffer->GetBuffer(), 6);
}

void RayTracer::ReleaseDescriptorSet()
{
    SAFE_DELETE(m_pDescriptorSet);
}

void RayTracer::ReloadShader()
{
    static bool bIsCompiling = false;

    if (!bIsCompiling)
    {
        bIsCompiling = true;

        std::async(std::launch::async, [this]()
        {
            // Compile the shaders
         #if PLATFORM_WINDOWS
            auto result = std::system("res\\compile_shaders.bat");
         #elif PLATFORM_MAC
            auto result = std::system("res/compile_shaders.command");
         #endif
            if (result != 0)
            {
                std::cout << "FAILED to Compile Shaders\n";
                return false;
            }

            // Upload the new shaders
            std::cout << "Compiled Shaders Successfully\n";

            // Create shader and pipeline
            ShaderModule* pComputeShader = ShaderModule::CreateFromFile(m_pDevice, "main", "res/shaders/raytracer.spv");
            if (!pComputeShader)
            {
                std::cout << "FAILED to create ComputeShader\n";
                return false;
            }

            ComputePipelineStateParams pipelineParams = {};
            pipelineParams.pShader         = pComputeShader;
            pipelineParams.pPipelineLayout = m_pPipelineLayout;

            ComputePipeline* pComputePipeline  = ComputePipeline::Create(m_pDevice, pipelineParams);
            if (!pComputePipeline)
            {
                std::cout << "FAILED to create ComputePipeline\n";
                return false;
            }

            SAFE_DELETE(pComputeShader);

            m_pDevice->WaitForIdle();
            m_pPipeline.store(pComputePipeline);

            bIsCompiling = false;
        });
    }
}
