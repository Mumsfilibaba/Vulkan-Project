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
    , m_bResetImage(true)
{
}

void RayTracer::Init(Device* pDevice, Swapchain* pSwapchain)
{
    // Set device
    m_pDevice    = pDevice;
    m_pSwapchain = pSwapchain;

    // Create DescriptorSetLayout
    constexpr uint32_t numBindings = 9;
    VkDescriptorSetLayoutBinding bindings[numBindings];
    bindings[0].binding            = 0;
    bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[0].descriptorCount    = 1;
    bindings[0].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    bindings[1].binding            = 1;
    bindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
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
    bindings[4].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
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

    bindings[7].binding            = 7;
    bindings[7].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[7].descriptorCount    = 1;
    bindings[7].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[7].pImmutableSamplers = nullptr;
    
    bindings[8].binding            = 8;
    bindings[8].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[8].descriptorCount    = 1;
    bindings[8].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[8].pImmutableSamplers = nullptr;

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
    ShaderModule* pComputeShader = ShaderModule::CreateFromFile(m_pDevice, "main", RESOURCE_PATH"/shaders/raytracer.spv");

    ComputePipelineStateParams pipelineParams = {};
    pipelineParams.pShader         = pComputeShader;
    pipelineParams.pPipelineLayout = m_pPipelineLayout;
    
    m_pPipeline = ComputePipeline::Create(m_pDevice, pipelineParams);
    assert(m_pPipeline != nullptr);

    delete pComputeShader;
   
    // Create DescriptorPool
    DescriptorPoolParams poolParams;
    poolParams.NumUniformBuffers = 3;
    poolParams.NumStorageImages  = 2;
    poolParams.NumStorageBuffers = 4;
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
  
    // QuadBuffer
    BufferParams quadBufferParams;
    quadBufferParams.Size             = sizeof(Quad) * MAX_QUAD;
    quadBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    quadBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_Quads.reserve(MAX_QUAD);
    //m_Quads.push_back(
    //{
    //    glm::vec4(-2.0f, 0.0f, -2.0f, 0.0f),
    //    glm::vec4( 0.0f, 0.0f,  4.0f, 0.0f),
    //    glm::vec4( 4.0f, 0.0f,  0.0f, 0.0f),
    //    0
    //});
    //m_Quads.push_back(
    //{
    //    glm::vec4(-2.0f, 0.0f, -2.0f, 0.0f),
    //    glm::vec4( 0.0f, 4.0f,  0.0f, 0.0f),
    //    glm::vec4( 4.0f, 0.0f,  0.0f, 0.0f),
    //    0
    //});
    //m_Quads.push_back(
    //{
    //    glm::vec4(-2.0f, 4.0f, -2.0f, 0.0f),
    //    glm::vec4( 0.0f, 0.0f,  4.0f, 0.0f),
    //    glm::vec4( 4.0f, 0.0f,  0.0f, 0.0f),
    //    0
    //});
    //m_Quads.push_back(
    //{
    //    glm::vec4(-2.0f, 0.0f,  2.0f, 0.0f),
    //    glm::vec4( 0.0f, 4.0f,  0.0f, 0.0f),
    //    glm::vec4( 0.0f, 0.0f, -4.0f, 0.0f),
    //    1
    //});
    //m_Quads.push_back(
    //{
    //    glm::vec4( 2.0f, 4.0f, -2.0f, 0.0f),
    //    glm::vec4( 0.0f,-4.0f,  0.0f, 0.0f),
    //    glm::vec4( 0.0f, 0.0f,  4.0f, 0.0f),
    //    2
    //});
    //m_Quads.push_back(
    //{
    //    glm::vec4(-0.75f, 3.995f, -0.75f, 0.0f),
    //    glm::vec4(  0.0f,   0.0f,   1.5f, 0.0f),
    //    glm::vec4(  1.5f,   0.0f,   0.0f, 0.0f),
    //    4
    //});

    m_pQuadBuffer = Buffer::Create(m_pDevice, quadBufferParams, m_pDeviceAllocator);
    assert(m_pQuadBuffer != nullptr);
    
    // SphereBuffer
    BufferParams sphereBufferParams;
    sphereBufferParams.Size             = sizeof(Sphere) * MAX_SPHERES;
    sphereBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    sphereBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_Spheres.reserve(MAX_SPHERES);
    m_Spheres.push_back({ glm::vec3(0.0f, -100.5f, 0.0f), 100.0f, 0 });
    m_Spheres.push_back({ glm::vec3(0.0f,    0.0f, 1.0f),   0.5f, 1 });

    m_Spheres.push_back({ glm::vec3(-1.0f, 0.0f, 1.0f), 0.5f, 2 });
    m_Spheres.push_back({ glm::vec3( 1.0f, 0.0f, 1.0f), 0.5f, 3 });

    m_pSphereBuffer = Buffer::Create(m_pDevice, sphereBufferParams, m_pDeviceAllocator);
    assert(m_pSphereBuffer != nullptr);

    // PlaneBuffer
    BufferParams planeBufferParams;
    planeBufferParams.Size             = sizeof(Plane) * MAX_PLANES;
    planeBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    planeBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_Planes.reserve(MAX_PLANES);

    m_pPlaneBuffer = Buffer::Create(m_pDevice, planeBufferParams, m_pDeviceAllocator);
    assert(m_pPlaneBuffer != nullptr);

    // MaterialBuffer
    BufferParams materialBufferParams;
    materialBufferParams.Size             = sizeof(Material) * MAX_MATERIALS;
    materialBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    materialBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_Materials.reserve(MAX_MATERIALS);
    m_Materials.push_back(
    {
        glm::vec4(0.8f, 0.8f, 0.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        MATERIAL_LAMBERTIAN,
        1.0f,
        0.0f,
    });
    m_Materials.push_back(
    {
        glm::vec4(0.7f, 0.3f, 0.3f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        MATERIAL_LAMBERTIAN,
        1.0f,
        0.0f,
    });
    m_Materials.push_back(
    {
        glm::vec4(0.8f, 0.8f, 0.8f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        MATERIAL_DIELECTRIC,
        0.3f,
        1.5f,
    });
    m_Materials.push_back(
    {
        glm::vec4(0.8f, 0.6f, 0.2f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        MATERIAL_METAL,
        1.0f,
        0.0f,
    });
    //m_Materials.push_back(
    //{
    //    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
    //    glm::vec4(2.0f, 2.0f, 2.0f, 1.0f),
    //    0.0f,
    //    MATERIAL_EMISSIVE
    //});
    //m_Materials.push_back(
    //{
    //    glm::vec4(0.02f, 0.02f, 0.99f, 1.0f),
    //    glm::vec4( 0.0f,  0.0f,  0.0f, 0.0f),
    //    0.99f,
    //    MATERIAL_LAMBERTIAN
    //});

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
    
    // Camera Movement
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

    // Camera rotation
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

    m_Camera.Rotate(rotation);
    
    // Check if we moved and then we reset the image
    if (glm::length(rotation) > 0.0f || glm::length(translation) > 0.0f)
    {
        m_bResetImage = true;
    }

    // Reload shaders
    if (Input::IsKeyDown(GLFW_KEY_R))
    {
        ReloadShader();
    }
    
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
    pCurrentCommandBuffer->TransitionImage(m_pAccumulationTexture->GetImage(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

    if (m_bResetImage)
    {
        VkClearColorValue clearColor = {};
        clearColor.float32[0] = 0.0f;
        clearColor.float32[1] = 0.0f;
        clearColor.float32[2] = 0.0f;
        clearColor.float32[3] = 1.0f;

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount     = 1;
        subresourceRange.baseMipLevel   = 0;
        subresourceRange.levelCount     = 1;

        pCurrentCommandBuffer->ClearColorImage(m_pAccumulationTexture->GetImage(), VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &subresourceRange);

        m_bResetImage = false;
        m_NumSamples  = 1;
    }
    else
    {
        m_NumSamples++;
    }

    // Update CameraBuffer
    CameraBuffer cameraBuffer = {};
    cameraBuffer.Projection = m_Camera.GetProjectionMatrix();
    cameraBuffer.View       = m_Camera.GetViewMatrix();
    cameraBuffer.Position   = glm::vec4(m_Camera.GetPosition(), 0.0f);
    cameraBuffer.Forward    = glm::vec4(m_Camera.GetForward(), 0.0f);

    pCurrentCommandBuffer->UpdateBuffer(m_pCameraBuffer, 0, sizeof(CameraBuffer), &cameraBuffer);

    // Update RandomBuffer
    constexpr uint32_t maxSamples = 16;
    static uint32_t randFrameIndex = 0;
    randFrameIndex++;
 
    RandomBuffer randomBuffer = {};
    randomBuffer.FrameIndex  = randFrameIndex;
    randomBuffer.SampleIndex = randFrameIndex % maxSamples;
    randomBuffer.NumSamples  = m_NumSamples;

    pCurrentCommandBuffer->UpdateBuffer(m_pRandomBuffer, 0, sizeof(RandomBuffer), &randomBuffer);

    // Update Scene
    SceneBuffer sceneBuffer = {};
    sceneBuffer.NumQuads     = m_Quads.size();
    sceneBuffer.NumSpheres   = m_Spheres.size();
    sceneBuffer.NumPlanes    = m_Planes.size();
    sceneBuffer.NumMaterials = m_Materials.size();

    pCurrentCommandBuffer->UpdateBuffer(m_pSceneBuffer, 0, sizeof(SceneBuffer), &sceneBuffer);
    
    if (!m_Quads.empty())
    {
        pCurrentCommandBuffer->UpdateBuffer(m_pQuadBuffer, 0, sizeof(Quad) * m_Quads.size(), m_Quads.data());
    }
    if (!m_Spheres.empty())
    {
        pCurrentCommandBuffer->UpdateBuffer(m_pSphereBuffer, 0, sizeof(Sphere) * m_Spheres.size(), m_Spheres.data());
    }
    if (!m_Planes.empty())
    {
        pCurrentCommandBuffer->UpdateBuffer(m_pPlaneBuffer, 0, sizeof(Plane) * m_Planes.size(), m_Planes.data());
    }
    if (!m_Materials.empty())
    {
        pCurrentCommandBuffer->UpdateBuffer(m_pMaterialBuffer, 0, sizeof(Material) * m_Materials.size(), m_Materials.data());
    }

    // Bind pipeline and descriptorSet
    pCurrentCommandBuffer->BindComputePipelineState(m_pPipeline.load());
    pCurrentCommandBuffer->BindComputeDescriptorSet(m_pPipelineLayout, m_pDescriptorSet);
    
    // Dispatch
    const uint32_t Threads = 16;
    VkExtent2D dispatchSize = { Math::AlignUp(m_pSceneTexture->GetWidth(), Threads) / Threads, Math::AlignUp(m_pSceneTexture->GetHeight(), Threads) / Threads };
    pCurrentCommandBuffer->Dispatch(dispatchSize.width, dispatchSize.height, 1);

    pCurrentCommandBuffer->TransitionImage(m_pSceneTexture->GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    pCurrentCommandBuffer->TransitionImage(m_pAccumulationTexture->GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    pCurrentCommandBuffer->WriteTimestamp(pCurrentTimestampQuery, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1);
    pCurrentCommandBuffer->End();

    m_pDevice->ExecuteGraphics(pCurrentCommandBuffer, nullptr, nullptr);
}

void RayTracer::OnRenderUI()
{
    // Setup Dockspace
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
    
    // Scene Settings
    if (ImGui::Begin("Scene"))
    {
        ImGui::Text("Performance:");
        ImGui::Separator();
        
        ImGui::Text("CPU Time %.4f", m_LastCPUTime);
        ImGui::Text("GPU Time %.4f", m_LastGPUTime);

        ImGui::NewLine();
        
        ImGui::Text("Image:");
        ImGui::Separator();
        
        if (ImGui::Button("Clear Image"))
        {
            m_bResetImage = true;
        }
        
        ImGui::NewLine();
        
        ImGui::Text("Objects:");
        ImGui::Separator();

        uint32_t index = 1;
        for (Sphere& sphere : m_Spheres)
        {
            ImGui::Text("Sphere %d", index++);
            ImGui::InputFloat3("Position", &sphere.Position.x);
            ImGui::InputFloat("Radius", &sphere.Radius);
        }
        
        ImGui::NewLine();
        
        ImGui::Text("Materials:");
        ImGui::Separator();
        
        ImGui::End();
    }
    
    // Viewport
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
    SAFE_DELETE(m_pQuadBuffer);
    SAFE_DELETE(m_pSphereBuffer);
    SAFE_DELETE(m_pPlaneBuffer);
    SAFE_DELETE(m_pMaterialBuffer);

    ReleaseDescriptorSet();

    SAFE_DELETE(m_pDescriptorPool);
    SAFE_DELETE(m_pPipeline);
    SAFE_DELETE(m_pPipelineLayout);
    SAFE_DELETE(m_pDescriptorSetLayout);
    SAFE_DELETE(m_pDeviceAllocator);
    SAFE_DELETE(m_pAccumulationTexture);
    SAFE_DELETE(m_pAccumulationTextureView);
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
        
        SAFE_DELETE(m_pAccumulationTexture);
        SAFE_DELETE(m_pAccumulationTextureView);
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
    textureParams.Usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    textureParams.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Accumulation texture
    m_pAccumulationTexture = Texture::Create(m_pDevice, textureParams);
    assert(m_pAccumulationTexture != nullptr);
    SetDebugName(m_pDevice->GetDevice(), "AccumulationTexture", reinterpret_cast<uint64_t>(m_pAccumulationTexture->GetImage()), VK_OBJECT_TYPE_IMAGE);

    {
        TextureViewParams textureViewParams = {};
        textureViewParams.pTexture = m_pAccumulationTexture;

        m_pAccumulationTextureView = TextureView::Create(m_pDevice, textureViewParams);
        assert(m_pAccumulationTextureView != nullptr);
        SetDebugName(m_pDevice->GetDevice(), "AccumulationTextureView", reinterpret_cast<uint64_t>(m_pAccumulationTextureView->GetImageView()), VK_OBJECT_TYPE_IMAGE_VIEW);
    }

    // Scene texture
    m_pSceneTexture = Texture::Create(m_pDevice, textureParams);
    assert(m_pSceneTexture != nullptr);
    SetDebugName(m_pDevice->GetDevice(), "SceneTexture", reinterpret_cast<uint64_t>(m_pSceneTexture->GetImage()), VK_OBJECT_TYPE_IMAGE);

    {
        TextureViewParams textureViewParams = {};
        textureViewParams.pTexture = m_pSceneTexture;

        m_pSceneTextureView = TextureView::Create(m_pDevice, textureViewParams);
        assert(m_pSceneTextureView != nullptr);
        SetDebugName(m_pDevice->GetDevice(), "SceneTextureView", reinterpret_cast<uint64_t>(m_pSceneTextureView->GetImageView()), VK_OBJECT_TYPE_IMAGE_VIEW);
    }

    // UI DescriptorSet
    m_pSceneTextureDescriptorSet = GUI::AllocateTextureID(m_pSceneTextureView);
    assert(m_pSceneTextureDescriptorSet != nullptr);

    // Descriptor set for when tracing
    CreateDescriptorSet();
    
    // When we have resized we need to clear the image as well
    m_bResetImage = true;
}

void RayTracer::CreateDescriptorSet()
{
    m_pDescriptorSet = DescriptorSet::Create(m_pDevice, m_pDescriptorPool, m_pDescriptorSetLayout);
    assert(m_pDescriptorSet != nullptr);

    m_pDescriptorSet->BindStorageImage(m_pSceneTextureView->GetImageView(), 0);
    m_pDescriptorSet->BindStorageImage(m_pAccumulationTextureView->GetImageView(), 1);
    m_pDescriptorSet->BindUniformBuffer(m_pCameraBuffer->GetBuffer(), 2);
    m_pDescriptorSet->BindUniformBuffer(m_pRandomBuffer->GetBuffer(), 3);
    m_pDescriptorSet->BindUniformBuffer(m_pSceneBuffer->GetBuffer(), 4);
    m_pDescriptorSet->BindStorageBuffer(m_pQuadBuffer->GetBuffer(), 5);
    m_pDescriptorSet->BindStorageBuffer(m_pSphereBuffer->GetBuffer(), 6);
    m_pDescriptorSet->BindStorageBuffer(m_pPlaneBuffer->GetBuffer(), 7);
    m_pDescriptorSet->BindStorageBuffer(m_pMaterialBuffer->GetBuffer(), 8);
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
            auto result = std::system(SHADER_SCRIPT_PATH);
            if (result != 0)
            {
                std::cout << "FAILED to Compile Shaders\n";
                bIsCompiling = false;
                return false;
            }

            // Upload the new shaders
            std::cout << "Compiled Shaders Successfully\n";

            // Create shader and pipeline
            ShaderModule* pComputeShader = ShaderModule::CreateFromFile(m_pDevice, "main", RESOURCE_PATH"/shaders/raytracer.spv");
            if (!pComputeShader)
            {
                std::cout << "FAILED to create ComputeShader\n";
                bIsCompiling = false;
                return false;
            }

            ComputePipelineStateParams pipelineParams = {};
            pipelineParams.pShader         = pComputeShader;
            pipelineParams.pPipelineLayout = m_pPipelineLayout;

            ComputePipeline* pComputePipeline  = ComputePipeline::Create(m_pDevice, pipelineParams);
            if (!pComputePipeline)
            {
                std::cout << "FAILED to create ComputePipeline\n";
                SAFE_DELETE(pComputeShader);
                bIsCompiling = false;
                return false;
            }

            m_pDevice->WaitForIdle();
            pComputePipeline = m_pPipeline.exchange(pComputePipeline);

            SAFE_DELETE(pComputeShader);
            SAFE_DELETE(pComputePipeline);
            
            m_bResetImage = true;
            bIsCompiling  = false;
            return true;
        });
    }
}
