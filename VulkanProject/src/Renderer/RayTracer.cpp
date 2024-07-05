#include "RayTracer.h"
#include "Application.h"
#include "Input.h"
#include "MathHelper.h"
#include "GUI.h"
#include "TextureResource.h"
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
#include <glm/gtc/type_ptr.hpp>

FRayTracer::FRayTracer()
    : m_pDevice(nullptr)
    , m_pSwapchain(nullptr)
    , m_pRayTracingPipeline()
    , m_pRayTracingPipelineLayout(nullptr)
    , m_pRayTracingDescriptorSetLayout(nullptr)
    , m_pDeviceAllocator(nullptr)
    , m_pDescriptorPool(nullptr)
    , m_pRayTracingDescriptorSet0(nullptr)
    , m_CommandBuffers()
    , m_TimestampQueries()
    , m_pCameraBuffer(nullptr)
    , m_pRandomBuffer(nullptr)
    , m_pSceneBuffer(nullptr)
    , m_pSphereBuffer(nullptr)
    , m_pQuadBuffer(nullptr)
    , m_pTriangleBuffer(nullptr)
    , m_pMeshBuffer(nullptr)
    , m_pVertexBuffer(nullptr)
    , m_pMaterialBuffer(nullptr)
    , m_pSceneTexture1(nullptr)
    , m_pSceneTextureView1(nullptr)
    , m_pSceneTexture0(nullptr)
    , m_pSceneTextureView0(nullptr)
    , m_pOutputTexture(nullptr)
    , m_pOutputTextureView(nullptr)
    , m_pOutputTextureDescriptorSet(nullptr)
    , m_pSkybox(nullptr)
    , m_pSkyboxSampler(nullptr)
    , m_pScene(nullptr)
    , m_bResetImage(false)
    , m_FrameIndex(0)
    , m_LastCPUTime(0.0f)
    , m_LastGPUTime(0.0f)
    , m_ViewportWidth(0)
    , m_ViewportHeight(0)
    , m_bViewportHasFocus(false)
{
}

FRayTracer::~FRayTracer()
{
    SAFE_DELETE(m_pScene);
}

void FRayTracer::Init(FDevice* pDevice, FSwapchain* pSwapchain)
{
    // Set device
    m_pDevice    = pDevice;
    m_pSwapchain = pSwapchain;

    // Init the textureloader
    FTextureResource::InitLoader(m_pDevice);
    
    // Create skybox
    m_pSkybox = FTextureResource::LoadCubeMapFromPanoramaFile(m_pDevice, RESOURCE_PATH"/textures/arches.hdr");
    assert(m_pSkybox != nullptr);

    // Skybox Sampler
    {
        FSamplerParams SamplerParams = {};
        SamplerParams.MagFilter     = VK_FILTER_LINEAR;
        SamplerParams.MinFilter     = VK_FILTER_LINEAR;
        SamplerParams.MipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        SamplerParams.AddressModeU  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SamplerParams.AddressModeV  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SamplerParams.AddressModeW  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SamplerParams.MinLod        = 0;
        SamplerParams.MaxLod        = 1000;
        SamplerParams.MaxAnisotropy = 1.0f;
        
        m_pSkyboxSampler = FSampler::Create(pDevice, SamplerParams);
        assert(m_pSkyboxSampler != nullptr);
    }
    
    // Tonemap Sampler
    {
        FSamplerParams SamplerParams = {};
        SamplerParams.MagFilter     = VK_FILTER_NEAREST;
        SamplerParams.MinFilter     = VK_FILTER_NEAREST;
        SamplerParams.MipmapMode    = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        SamplerParams.AddressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        SamplerParams.AddressModeV  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        SamplerParams.AddressModeW  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        SamplerParams.MinLod        = 0;
        SamplerParams.MaxLod        = 1000;
        SamplerParams.MaxAnisotropy = 1.0f;
        
        m_pTonemapSampler = FSampler::Create(pDevice, SamplerParams);
        assert(m_pTonemapSampler != nullptr);
    }
    
    // Create scene
    m_pScene = new FSphereScene(ESphereSceneType::Default);
    m_pScene->Initialize();

    // RenderPasses
    CreateRayTracingResources();
    CreateTonemappingResources();
    
    // Create all buffers
    CreateGlobalBuffers();
    
    // Create DescriptorPool
    FDescriptorPoolParams DescriptorPoolParams;
    DescriptorPoolParams.NumUniformBuffers        = 32;
    DescriptorPoolParams.NumStorageImages         = 32;
    DescriptorPoolParams.NumStorageBuffers        = 32;
    DescriptorPoolParams.NumCombinedImageSamplers = 32;
    DescriptorPoolParams.MaxSets                  = 4;
    
    m_pDescriptorPool = FDescriptorPool::Create(m_pDevice, DescriptorPoolParams);
    assert(m_pDescriptorPool != nullptr);

    // Create the scene texture
    m_ViewportWidth  = 0;
    m_ViewportHeight = 0;
    CreateOrResizeSceneTexture(1280, 720);

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

    // Timestamp queries
    FQueryParams QueryParams;
    QueryParams.QueryType  = VK_QUERY_TYPE_TIMESTAMP;
    QueryParams.QueryCount = 2;
    
    m_TimestampQueries.resize(ImageCount);
    for (size_t i = 0; i < m_TimestampQueries.size(); i++)
    {
        FQuery* pQuery = FQuery::Create(m_pDevice, QueryParams);
        pQuery->Reset();
        
        m_TimestampQueries[i] = pQuery;
    }
    
    // Allocator for GPU memory
    m_pDeviceAllocator = new FDeviceMemoryAllocator(m_pDevice);
}

void FRayTracer::Tick(float DeltaTime)
{
    m_LastCPUTime = DeltaTime * 1000.0f; // deltaTime is in seconds

    // Update scene image
    CreateOrResizeSceneTexture(m_ViewportWidth, m_ViewportHeight);

    // Camera Movement
    const float CameraSpeed = m_pScene->m_Settings.CameraSpeed;
    if (m_bViewportHasFocus)
    {
        glm::vec3 Translation(0.0f);
        if (FInput::IsKeyDown(GLFW_KEY_W))
        {
            Translation.z = CameraSpeed * DeltaTime;
        }
        else if (FInput::IsKeyDown(GLFW_KEY_S))
        {
            Translation.z = -CameraSpeed * DeltaTime;
        }

        if (FInput::IsKeyDown(GLFW_KEY_A))
        {
            Translation.x = CameraSpeed * DeltaTime;
        }
        else if (FInput::IsKeyDown(GLFW_KEY_D))
        {
            Translation.x = -CameraSpeed * DeltaTime;
        }

        m_pScene->m_Camera.Move(Translation);

        // Camera rotation
        constexpr float CameraRotationSpeed = glm::pi<float>() / 2;

        glm::vec3 Rotation(0.0f);
        if (FInput::IsKeyDown(GLFW_KEY_LEFT))
        {
            Rotation.y = -CameraRotationSpeed * DeltaTime;
        }
        else if (FInput::IsKeyDown(GLFW_KEY_RIGHT))
        {
            Rotation.y = CameraRotationSpeed * DeltaTime;
        }

        if (FInput::IsKeyDown(GLFW_KEY_UP))
        {
            Rotation.x = -CameraRotationSpeed * DeltaTime;
        }
        else if (FInput::IsKeyDown(GLFW_KEY_DOWN))
        {
            Rotation.x = CameraRotationSpeed * DeltaTime;
        }

        m_pScene->m_Camera.Rotate(Rotation);

        // Check if we moved and then we reset the image
        if (glm::length(Rotation) > 0.0f || glm::length(Translation) > 0.0f)
        {
            m_bResetImage = true;
        }

        // Reload shaders
        if (FInput::IsKeyDown(GLFW_KEY_R))
        {
            ReloadShader();
        }
    }

    // Update
    m_pScene->m_Camera.Update(m_pScene->m_Settings.FieldOfView, m_pSceneTexture0->GetWidth(), m_pSceneTexture0->GetHeight(), 0.1f, 100.0f);

    // Draw
    uint32_t FrameIndex = m_pSwapchain->GetCurrentBackBufferIndex();
    FQuery*         pCurrentTimestampQuery = m_TimestampQueries[FrameIndex];
    FCommandBuffer* pCurrentCommandBuffer  = m_CommandBuffers[FrameIndex];

    // Reset CommandBuffer
    pCurrentCommandBuffer->Reset();

    // Prepare timestamps
    constexpr uint32_t TimestampCount = 2;
    uint64_t Timestamps[TimestampCount];
    ZERO_MEMORY(Timestamps, sizeof(uint64_t) * TimestampCount);

    pCurrentTimestampQuery->GetData(0, 2, sizeof(uint64_t) * TimestampCount, &Timestamps, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
    pCurrentTimestampQuery->Reset();

    const double TimestampPeriod = double(m_pDevice->GetTimestampPeriod());
    const double GpuTiming   = (double(Timestamps[1]) - double(Timestamps[0])) * TimestampPeriod;
    const double GpuTimingMS = GpuTiming / 1000000.0;
    m_LastGPUTime = static_cast<float>(GpuTimingMS);

    // Begin CommandBuffer
    pCurrentCommandBuffer->Begin();
    pCurrentCommandBuffer->WriteTimestamp(pCurrentTimestampQuery, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);

    pCurrentCommandBuffer->TransitionImage(m_pSceneTexture0->GetImage(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
    pCurrentCommandBuffer->TransitionImage(m_pSceneTexture1->GetImage(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

    if (m_bResetImage)
    {
        VkClearColorValue ClearColor = {};
        ClearColor.float32[0] = 0.0f;
        ClearColor.float32[1] = 0.0f;
        ClearColor.float32[2] = 0.0f;
        ClearColor.float32[3] = 1.0f;

        VkImageSubresourceRange SubresourceRange = {};
        SubresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        SubresourceRange.baseArrayLayer = 0;
        SubresourceRange.layerCount     = 1;
        SubresourceRange.baseMipLevel   = 0;
        SubresourceRange.levelCount     = 1;

        pCurrentCommandBuffer->ClearColorImage(m_pSceneTexture0->GetImage(), VK_IMAGE_LAYOUT_GENERAL, &ClearColor, 1, &SubresourceRange);
        pCurrentCommandBuffer->ClearColorImage(m_pSceneTexture1->GetImage(), VK_IMAGE_LAYOUT_GENERAL, &ClearColor, 1, &SubresourceRange);

        m_bResetImage = false;
        m_FrameIndex  = 0;
    }
    else
    {
        m_FrameIndex++;
    }

    // Update CameraBuffer
    FCameraBuffer CameraBuffer = {};
    CameraBuffer.Projection         = m_pScene->m_Camera.GetProjectionMatrix();
    CameraBuffer.View               = m_pScene->m_Camera.GetViewMatrix();
    CameraBuffer.Position           = glm::vec4(m_pScene->m_Camera.GetPosition(), 0.0f);
    CameraBuffer.Forward            = glm::vec4(m_pScene->m_Camera.GetForward(), 0.0f);
    CameraBuffer.FieldOfViewDegrees = Math::ToDegrees(m_pScene->m_Camera.GetFieldOfView());
    
    pCurrentCommandBuffer->UpdateBuffer(m_pCameraBuffer, 0, sizeof(FCameraBuffer), &CameraBuffer);

    // Update RandomBuffer
    constexpr uint32_t MaxSamples = 16;

    FRandomBuffer RandomBuffer = {};
    RandomBuffer.FrameIndex  = m_FrameIndex;
    RandomBuffer.HaltonIndex = m_FrameIndex % MaxSamples;

    pCurrentCommandBuffer->UpdateBuffer(m_pRandomBuffer, 0, sizeof(FRandomBuffer), &RandomBuffer);

    // Update Tonemapping Settings
    FTonemappingBuffer TonemappingBuffer = {};
    TonemappingBuffer.Exposure = m_pScene->m_Settings.Exposure;
    
    pCurrentCommandBuffer->UpdateBuffer(m_pTonemappingBuffer, 0, sizeof(FTonemappingBuffer), &TonemappingBuffer);
    
    // Update Scene
    FSceneBuffer SceneBuffer = {};
    SceneBuffer.NumQuads       = m_pScene->m_Quads.size();
    SceneBuffer.NumSpheres     = m_pScene->m_Spheres.size();
    SceneBuffer.NumMaterials   = m_pScene->m_Materials.size();
    SceneBuffer.NumMeshes      = m_pScene->m_Meshes.size();
    SceneBuffer.NumBvhNodes    = m_pScene->m_AccelerationStructure.m_BoundingBoxes.size();
    SceneBuffer.NumTriangles   = m_pScene->m_AccelerationStructure.m_Triangles.size();
    SceneBuffer.BackgroundType = m_pScene->m_Settings.BackgroundType;
    SceneBuffer.NumBounces     = m_pScene->m_Settings.NumBounces;
    SceneBuffer.ViewMode       = static_cast<uint32_t>(m_pScene->m_Settings.ViewMode);

    pCurrentCommandBuffer->UpdateBuffer(m_pSceneBuffer, 0, sizeof(FSceneBuffer), &SceneBuffer);
        
    UpdateGlobalBuffers(pCurrentCommandBuffer);

    // Bind pipeline and descriptorSet
    pCurrentCommandBuffer->BindComputePipelineState(m_pRayTracingPipeline.load());
    
    const uint64_t Frame = (m_FrameIndex % 2);
    if (Frame == 0)
    {
        pCurrentCommandBuffer->BindComputeDescriptorSet(m_pRayTracingPipelineLayout, m_pRayTracingDescriptorSet0);
    }
    else
    {
        pCurrentCommandBuffer->BindComputeDescriptorSet(m_pRayTracingPipelineLayout, m_pRayTracingDescriptorSet1);
    }

    // Dispatch RayTracing
    const uint32_t Threads = 16;
    VkExtent2D DispatchSize = { Math::AlignUp(m_pSceneTexture0->GetWidth(), Threads) / Threads, Math::AlignUp(m_pSceneTexture0->GetHeight(), Threads) / Threads };
    pCurrentCommandBuffer->Dispatch(DispatchSize.width, DispatchSize.height, 1);

    pCurrentCommandBuffer->TransitionImage(m_pSceneTexture0->GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    pCurrentCommandBuffer->TransitionImage(m_pSceneTexture1->GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Begin renderpass
    VkClearValue ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    pCurrentCommandBuffer->BeginRenderPass(m_pTonemappingRenderPass, m_pTonemappingFramebuffer, &ClearColor, 1);
    
    // Set viewport
    VkViewport Viewport = { 0.0f, 0.0f, float(m_ViewportWidth), float(m_ViewportHeight), 0.0f, 1.0f };
    pCurrentCommandBuffer->SetViewport(Viewport);
    
    VkRect2D scissor = { { 0, 0}, { m_ViewportWidth, m_ViewportHeight } };
    pCurrentCommandBuffer->SetScissorRect(scissor);
    
    // Bind pipeline
    pCurrentCommandBuffer->BindGraphicsPipelineState(m_pTonemappingPipeline);

    // Perform tonemapping
    if (Frame == 0)
    {
        pCurrentCommandBuffer->BindGraphicsDescriptorSet(m_pTonemappingPipelineLayout, m_pTonemappingDescriptorSet0);
    }
    else
    {
        pCurrentCommandBuffer->BindGraphicsDescriptorSet(m_pTonemappingPipelineLayout, m_pTonemappingDescriptorSet1);
    }

    // Draw
    pCurrentCommandBuffer->DrawInstanced(3, 1, 0, 0);
    
    // End renderpass
    pCurrentCommandBuffer->EndRenderPass();

    // End CommandBuffer
    pCurrentCommandBuffer->WriteTimestamp(pCurrentTimestampQuery, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1);
    pCurrentCommandBuffer->End();

    m_pDevice->ExecuteGraphics(pCurrentCommandBuffer, nullptr, nullptr);
}

void FRayTracer::OnRenderUI()
{
    // Setup DockSpace
    static ImGuiDockNodeFlags DockspaceFlags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoDocking;

    const ImGuiViewport* pMainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(pMainViewport->WorkPos);
    ImGui::SetNextWindowSize(pMainViewport->WorkSize);
    ImGui::SetNextWindowViewport(pMainViewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    WindowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    WindowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (DockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
    {
        WindowFlags |= ImGuiWindowFlags_NoBackground;
    }

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", nullptr, WindowFlags);

    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);

    // Submit the DockSpace
    ImGuiIO& UIConfig = ImGui::GetIO();
    if (UIConfig.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID DockspaceID = ImGui::GetID("PathTracerDockspace");
        ImGui::DockSpace(DockspaceID, ImVec2(0.0f, 0.0f), DockspaceFlags);
    }

    ImGui::End();

    // Scene Settings
    if (ImGui::Begin("Scene"))
    {
        ImGui::Text("Performance:");
        ImGui::Separator();

        const uint32_t CpuCurrentFPS = static_cast<uint32_t>(1000.0f / m_LastCPUTime);
        ImGui::Text("[CPU FPS] %u", CpuCurrentFPS);
        ImGui::Text("[CPU Time] %.4f", m_LastCPUTime);
        
        const uint32_t GpuCurrentFPS = static_cast<uint32_t>(1000.0f / m_LastGPUTime);
        ImGui::Text("[GPU FPS] %u", GpuCurrentFPS);
        ImGui::Text("[GPU Time] %.4f", m_LastGPUTime);
        
        ImGui::Text("Current Resolution: %dx%d", m_ViewportWidth, m_ViewportHeight);

        ImGui::NewLine();

        ImGui::Text("Image:");
        ImGui::Separator();

        // Select the view-mode
        {
            static const char* ViewModes[] =
            {
                "Render",
                "Normals",
                "TopBVH"
            };

            static int CurrentViewMode = static_cast<int>(m_pScene->m_Settings.ViewMode);
            static int PrevViewMode = CurrentViewMode;
            
            ImGui::Combo("ViewMode", &CurrentViewMode, ViewModes, IM_ARRAYSIZE(ViewModes));
            
            if (CurrentViewMode != PrevViewMode)
            {
                if (CurrentViewMode == 0)
                {
                    m_pScene->m_Settings.ViewMode = EViewMode::Render;
                }
                else if (CurrentViewMode == 1)
                {
                    m_pScene->m_Settings.ViewMode = EViewMode::Normals;
                }
                else if (CurrentViewMode == 2)
                {
                    m_pScene->m_Settings.ViewMode = EViewMode::TopBVH;
                }
                
                PrevViewMode = CurrentViewMode;
                m_bResetImage = true;
            }
        }
        
        // Clear the image
        if (ImGui::Button("Clear Image"))
        {
            m_bResetImage = true;
        }

        ImGui::NewLine();

        ImGui::Text("Scene:");
        ImGui::Separator();

        // Scene selector
        {
            static const char* Scenes[] =
            {
                "Spheres Default",
                "CornellBox",
                "Triangles",
                "Sponza",
                "Polished Glass Spheres",
                "Rough Colored Glass Spheres",
                "Rough Transparent Glass Spheres",
            };

            static int CurrentScene = 0;
            static int PrevScene    = 0;
            ImGui::Combo("Current Scene", &CurrentScene, Scenes, IM_ARRAYSIZE(Scenes));

            if (PrevScene != CurrentScene)
            {
                const EViewMode ViewMode = m_pScene->m_Settings.ViewMode;
                if (CurrentScene == 0) // Change to Sphere-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = new FSphereScene(ESphereSceneType::Default);
                    m_pScene->Initialize();
                    m_bResetImage = true;
                }
                else if (CurrentScene == 1) // Change to CornellBox-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = new FCornellBoxScene();
                    m_pScene->Initialize();
                    m_bResetImage = true;
                }
                else if (CurrentScene == 2) // Change to Triangles-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = new FModelScene(EModelSceneType::Default);
                    m_pScene->Initialize();
                    m_bResetImage = true;
                }
                else if (CurrentScene == 3) // Change to "Polished Glass Sphere"-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = new FModelScene(EModelSceneType::Sponza);
                    m_pScene->Initialize();
                    m_bResetImage = true;
                }
                else if (CurrentScene == 4) // Change to "Polished Glass Sphere"-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = new FSphereScene(ESphereSceneType::PolishedGlass);
                    m_pScene->Initialize();
                    m_bResetImage = true;
                }
                else if (CurrentScene == 5) // Change to "Rough Colored Glass Spheres"-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = new FSphereScene(ESphereSceneType::ColoredRoughGlass);
                    m_pScene->Initialize();
                    m_bResetImage = true;
                }
                else if (CurrentScene == 6) // Change to "Rough Transparent Glass Spheres"-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = new FSphereScene(ESphereSceneType::RoughGlass);
                    m_pScene->Initialize();
                    m_bResetImage = true;
                }

                m_pScene->m_Settings.ViewMode = ViewMode;
                PrevScene = CurrentScene;
            }
            
            // Background
            static const char* Background[] =
            {
                "None",
                "Gradient",
                "Skybox",
            };
            
            int CurrentBG = m_pScene->m_Settings.BackgroundType;
            static int PrevBG = CurrentBG;
            ImGui::Combo("Background", &CurrentBG, Background, IM_ARRAYSIZE(Background));
            
            if (PrevBG != CurrentBG)
            {
                if (CurrentBG == 0)
                {
                    m_pScene->m_Settings.BackgroundType = BACKGROUND_TYPE_NONE;
                }
                else if (CurrentBG == 1)
                {
                    m_pScene->m_Settings.BackgroundType = BACKGROUND_TYPE_GRADIENT;
                }
                else if (CurrentBG == 2)
                {
                    m_pScene->m_Settings.BackgroundType = BACKGROUND_TYPE_SKYBOX;
                }
                
                m_bResetImage = true;
                PrevBG = CurrentBG;
            }

            float Exposure = m_pScene->m_Settings.Exposure;
            if (ImGui::DragFloat("Exposure", &Exposure, 0.01f, 0.0f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
            {
                m_pScene->m_Settings.Exposure = Exposure;
                m_bResetImage = true;
            }
            
            float FieldOfView = m_pScene->m_Settings.FieldOfView;
            if (ImGui::DragFloat("FieldOfView", &FieldOfView, 0.1f, 30.0f, 120.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
            {
                m_pScene->m_Settings.FieldOfView = FieldOfView;
                m_bResetImage = true;
            }
            
            int NumBounces = m_pScene->m_Settings.NumBounces;
            if (ImGui::DragInt("Num Bounces", &NumBounces, 1, 1, 1024, "%d", ImGuiSliderFlags_AlwaysClamp))
            {
                m_pScene->m_Settings.NumBounces = NumBounces;
                m_bResetImage = true;
            }
        }

        // Reset the scene
        if (ImGui::Button("Reset Camera"))
        {
            m_pScene->Reset();
            m_bResetImage = true;
        }

        ImGui::NewLine();

        ImGui::Text("Objects:");
        ImGui::Separator();

        uint32_t ImguiID = 0;
        {
            uint32_t Index = 1;
            for (FShaderSphere& Sphere : m_pScene->m_Spheres)
            {
                ImGui::PushID(ImguiID++);

                ImGui::Text("Sphere %d", Index++);
                if (ImGui::DragFloat3("Position", glm::value_ptr(Sphere.Position), 0.1f))
                {
                    m_bResetImage = true;
                }
                if (ImGui::DragFloat("Radius", &Sphere.Radius, 0.01f))
                {
                    m_bResetImage = true;
                }

                ImGui::PopID();
                ImGui::Separator();
            }
        }

        {
            uint32_t Index = 1;
            for (FShaderQuad& Quad : m_pScene->m_Quads)
            {
                ImGui::PushID(ImguiID++);

                ImGui::Text("Quad %d", Index++);
                if (ImGui::DragFloat3("Position", glm::value_ptr(Quad.Position), 0.1f))
                {
                    m_bResetImage = true;
                }
                if (ImGui::DragFloat3("Edge0", glm::value_ptr(Quad.Edge0), 0.1f))
                {
                    m_bResetImage = true;
                }
                if (ImGui::DragFloat3("Edge1", glm::value_ptr(Quad.Edge1), 0.1f))
                {
                    m_bResetImage = true;
                }

                ImGui::PopID();
                ImGui::Separator();
            }
        }

        {
            uint32_t Index = 1;
            for (FShaderMesh& Mesh : m_pScene->m_Meshes)
            {
                ImGui::PushID(ImguiID++);

                ImGui::Text("TriangleMesh %d", Index++);
                
                ImGui::PopID();
                ImGui::Separator();
            }
        }

        ImGui::NewLine();

        ImGui::Text("Materials:");
        ImGui::Separator();

        {
            uint32_t Index = 1;
            for (FShaderMaterial& Material : m_pScene->m_Materials)
            {
                ImGui::PushID(ImguiID++);
                ImGui::Text("Material %d", Index++);
            
                // if (ImGui::ColorEdit3("Albedo", glm::value_ptr(material.Albedo)))
                if (ImGui::InputFloat3("AlbedoColor", glm::value_ptr(Material.AlbedoColor)))
                {
                    m_bResetImage = true;
                }
                // if (ImGui::ColorEdit3("Emissive", glm::value_ptr(material.Emissive)))
                if (ImGui::InputFloat3("EmissiveColor", glm::value_ptr(Material.EmissiveColor)))
                {
                    m_bResetImage = true;
                }
                // if (ImGui::ColorEdit3("Emissive", glm::value_ptr(material.Emissive)))
                if (ImGui::InputFloat3("SpecularColor", glm::value_ptr(Material.SpecularColor)))
                {
                    m_bResetImage = true;
                }
                // if (ImGui::ColorEdit3("Emissive", glm::value_ptr(material.AbsorbtionColor)))
                if (ImGui::InputFloat3("AbsorbtionColor", glm::value_ptr(Material.AbsorbtionColor)))
                {
                    m_bResetImage = true;
                }
                if (ImGui::DragFloat("SpecularChance", &Material.SpecularChance, 0.1f, 0.0f, 1.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
                {
                    m_bResetImage = true;
                }
                if (ImGui::DragFloat("SpecularRoughness", &Material.SpecularRoughness, 0.1f, 0.0f, 1.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
                {
                    m_bResetImage = true;
                }
                if (ImGui::DragFloat("RefractionChance", &Material.RefractionChance, 0.1f, 0.0f, 1.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
                {
                    m_bResetImage = true;
                }
                if (ImGui::DragFloat("RefractionRoughness", &Material.RefractionRoughness, 0.1f, 0.0f, 1.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
                {
                    m_bResetImage = true;
                }
                if (ImGui::DragFloat("IncidenceOfRefraction", &Material.IncidenceOfRefraction, 0.1f, 0.5f, 2.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
                {
                    m_bResetImage = true;
                }

                ImGui::PopID();
                ImGui::Separator();
            }
        }

        ImGui::End();
    }
    
    // Viewport
    ImGui::Begin("Viewport");

    m_bViewportHasFocus = ImGui::IsWindowFocused();
    m_ViewportWidth     = ImGui::GetContentRegionAvail().x;
    m_ViewportHeight    = ImGui::GetContentRegionAvail().y;

    if (m_pOutputTexture)
    {
        ImGui::Image(m_pOutputTextureDescriptorSet, { (float)m_pOutputTexture->GetWidth(), (float)m_pOutputTexture->GetHeight() });
    }
    
    ImGui::End();
}

void FRayTracer::Release()
{
    FTextureResource::ReleaseLoader();
    
    for (auto& CommandBuffer : m_CommandBuffers)
    {
        SAFE_DELETE(CommandBuffer);
    }

    m_CommandBuffers.clear();

    for (auto& Query : m_TimestampQueries)
    {
        SAFE_DELETE(Query);
    }

    m_TimestampQueries.clear();

    ReleaseDescriptorSets();

    SAFE_DELETE(m_pCameraBuffer);
    SAFE_DELETE(m_pRandomBuffer);
    SAFE_DELETE(m_pSceneBuffer);
    SAFE_DELETE(m_pTonemappingBuffer);
    SAFE_DELETE(m_pQuadBuffer);
    SAFE_DELETE(m_pSphereBuffer);
    SAFE_DELETE(m_pVertexBuffer);
    SAFE_DELETE(m_pTriangleBuffer);
    SAFE_DELETE(m_pMeshBuffer);
    SAFE_DELETE(m_pMaterialBuffer);
    SAFE_DELETE(m_pBvhBuffer);
    
    SAFE_DELETE(m_pSkybox);
    
    SAFE_DELETE(m_pSkyboxSampler);
    SAFE_DELETE(m_pTonemapSampler);

    SAFE_DELETE(m_pRayTracingPipeline);
    SAFE_DELETE(m_pRayTracingPipelineLayout);
    SAFE_DELETE(m_pRayTracingDescriptorSetLayout);
    
    SAFE_DELETE(m_pTonemappingRenderPass);
    SAFE_DELETE(m_pTonemappingPipeline);
    SAFE_DELETE(m_pTonemappingPipelineLayout);
    SAFE_DELETE(m_pTonemappingDescriptorSetLayout);

    SAFE_DELETE(m_pSceneTexture1);
    SAFE_DELETE(m_pSceneTextureView1);
    SAFE_DELETE(m_pSceneTexture0);
    SAFE_DELETE(m_pSceneTextureView0);
    SAFE_DELETE(m_pOutputTexture);
    SAFE_DELETE(m_pOutputTextureView);
    SAFE_DELETE(m_pTonemappingFramebuffer);

    SAFE_DELETE(m_pDescriptorPool);
    SAFE_DELETE(m_pDeviceAllocator);
}

void FRayTracer::OnWindowResize(uint32_t Width, uint32_t Height)
{
}

void FRayTracer::CreateRayTracingResources()
{
    // Create RayTracing DescriptorSetLayout
    constexpr uint32_t NumRayTracingBindings = 13;
    VkDescriptorSetLayoutBinding RayTracingBindings[NumRayTracingBindings];
    
    // Output Image
    RayTracingBindings[0].binding            = 0;
    RayTracingBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    RayTracingBindings[0].descriptorCount    = 1;
    RayTracingBindings[0].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[0].pImmutableSamplers = nullptr;

    // Accumulation image
    RayTracingBindings[1].binding            = 1;
    RayTracingBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    RayTracingBindings[1].descriptorCount    = 1;
    RayTracingBindings[1].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[1].pImmutableSamplers = nullptr;
    
    // Skybox
    RayTracingBindings[2].binding            = 2;
    RayTracingBindings[2].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    RayTracingBindings[2].descriptorCount    = 1;
    RayTracingBindings[2].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[2].pImmutableSamplers = nullptr;
    
    // Camera Buffer
    RayTracingBindings[3].binding            = 3;
    RayTracingBindings[3].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    RayTracingBindings[3].descriptorCount    = 1;
    RayTracingBindings[3].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[3].pImmutableSamplers = nullptr;
    
    // Random Buffer
    RayTracingBindings[4].binding            = 4;
    RayTracingBindings[4].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    RayTracingBindings[4].descriptorCount    = 1;
    RayTracingBindings[4].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[4].pImmutableSamplers = nullptr;

    // Scene Buffer
    RayTracingBindings[5].binding            = 5;
    RayTracingBindings[5].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    RayTracingBindings[5].descriptorCount    = 1;
    RayTracingBindings[5].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[5].pImmutableSamplers = nullptr;

    // Quads Buffer
    RayTracingBindings[6].binding            = 6;
    RayTracingBindings[6].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[6].descriptorCount    = 1;
    RayTracingBindings[6].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[6].pImmutableSamplers = nullptr;

    // Spheres Buffer
    RayTracingBindings[7].binding            = 7;
    RayTracingBindings[7].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[7].descriptorCount    = 1;
    RayTracingBindings[7].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[7].pImmutableSamplers = nullptr;

    // Materials Buffer
    RayTracingBindings[8].binding            = 8;
    RayTracingBindings[8].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[8].descriptorCount    = 1;
    RayTracingBindings[8].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[8].pImmutableSamplers = nullptr;
    
    // Vertex Buffer
    RayTracingBindings[9].binding            = 9;
    RayTracingBindings[9].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[9].descriptorCount    = 1;
    RayTracingBindings[9].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[9].pImmutableSamplers = nullptr;

    // Triangles Buffer
    RayTracingBindings[10].binding            = 10;
    RayTracingBindings[10].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[10].descriptorCount    = 1;
    RayTracingBindings[10].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[10].pImmutableSamplers = nullptr;
    
    // TriangleMeshes Buffer
    RayTracingBindings[11].binding            = 11;
    RayTracingBindings[11].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[11].descriptorCount    = 1;
    RayTracingBindings[11].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[11].pImmutableSamplers = nullptr;
    
    // Bvh Buffer
    RayTracingBindings[12].binding            = 12;
    RayTracingBindings[12].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[12].descriptorCount    = 1;
    RayTracingBindings[12].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[12].pImmutableSamplers = nullptr;

    FDescriptorSetLayoutParams RayTracingDescriptorSetLayoutParams;
    RayTracingDescriptorSetLayoutParams.pBindings   = RayTracingBindings;
    RayTracingDescriptorSetLayoutParams.numBindings = NumRayTracingBindings;

    m_pRayTracingDescriptorSetLayout = FDescriptorSetLayout::Create(m_pDevice, RayTracingDescriptorSetLayoutParams);
    assert(m_pRayTracingDescriptorSetLayout != nullptr);

    // Create RayTracing PipelineLayout
    FPipelineLayoutParams RayTracingPipelineLayoutParams;
    RayTracingPipelineLayoutParams.ppLayouts  = &m_pRayTracingDescriptorSetLayout;
    RayTracingPipelineLayoutParams.numLayouts = 1;

    m_pRayTracingPipelineLayout = FPipelineLayout::Create(m_pDevice, RayTracingPipelineLayoutParams);
    assert(m_pRayTracingPipelineLayout != nullptr);

    // Create RayTracing shader and pipeline
    FShaderModule* pComputeShader = FShaderModule::CreateFromFile(m_pDevice, "main", RESOURCE_PATH"/shaders/raytracer.spv");
    assert(pComputeShader != nullptr);
    
    FComputePipelineStateParams PipelineParams = {};
    PipelineParams.pShader         = pComputeShader;
    PipelineParams.pPipelineLayout = m_pRayTracingPipelineLayout;
    
    m_pRayTracingPipeline = FComputePipeline::Create(m_pDevice, PipelineParams);
    assert(m_pRayTracingPipeline != nullptr);

    delete pComputeShader;
}

void FRayTracer::CreateTonemappingResources()
{
    // Create Tonemapping DescriptorSetLayout
    constexpr uint32_t NumTonemappingBindings = 2;
    VkDescriptorSetLayoutBinding TonemappingBindings[NumTonemappingBindings];
    
    // Output Image
    TonemappingBindings[0].binding            = 0;
    TonemappingBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    TonemappingBindings[0].descriptorCount    = 1;
    TonemappingBindings[0].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    TonemappingBindings[0].pImmutableSamplers = nullptr;

    // Accumulation image
    TonemappingBindings[1].binding            = 1;
    TonemappingBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    TonemappingBindings[1].descriptorCount    = 1;
    TonemappingBindings[1].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    TonemappingBindings[1].pImmutableSamplers = nullptr;
    
    FDescriptorSetLayoutParams TonemappingDescriptorSetLayoutParams;
    TonemappingDescriptorSetLayoutParams.pBindings   = TonemappingBindings;
    TonemappingDescriptorSetLayoutParams.numBindings = NumTonemappingBindings;

    m_pTonemappingDescriptorSetLayout = FDescriptorSetLayout::Create(m_pDevice, TonemappingDescriptorSetLayoutParams);
    assert(m_pTonemappingDescriptorSetLayout != nullptr);

    // Create RayTracing PipelineLayout
    FPipelineLayoutParams ToneMappingPipelineLayoutParams;
    ToneMappingPipelineLayoutParams.ppLayouts  = &m_pTonemappingDescriptorSetLayout;
    ToneMappingPipelineLayoutParams.numLayouts = 1;
    
    m_pTonemappingPipelineLayout = FPipelineLayout::Create(m_pDevice, ToneMappingPipelineLayoutParams);
    assert(m_pTonemappingPipelineLayout != nullptr);
    
    // PipelineState, RenderPass and Shaders
    FShaderModule* pVertex = FShaderModule::CreateFromFile(m_pDevice, "main", RESOURCE_PATH"/shaders/fullscreenVS.spv");
    assert(pVertex != nullptr);
    
    FShaderModule* pFragment = FShaderModule::CreateFromFile(m_pDevice, "main", RESOURCE_PATH"/shaders/tonemap.spv");
    assert(pFragment != nullptr);
    
    FRenderPassAttachment Attachments[1];
    Attachments[0].Format        = VK_FORMAT_R8G8B8A8_UNORM;
    Attachments[0].InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    Attachments[0].FinalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    FRenderPassParams RenderPassParams = {};
    RenderPassParams.ColorAttachmentCount = 1;
    RenderPassParams.pColorAttachments    = Attachments;
    
    m_pTonemappingRenderPass = FRenderPass::Create(m_pDevice, RenderPassParams);
    assert(m_pTonemappingRenderPass != nullptr);
    
    FGraphicsPipelineStateParams TonemappingPipelineParams = {};
    TonemappingPipelineParams.pBindingDescriptions      = nullptr;
    TonemappingPipelineParams.BindingDescriptionCount   = 0;
    TonemappingPipelineParams.pAttributeDescriptions    = nullptr;
    TonemappingPipelineParams.AttributeDescriptionCount = 0;
    TonemappingPipelineParams.pVertexShader             = pVertex;
    TonemappingPipelineParams.pFragmentShader           = pFragment;
    TonemappingPipelineParams.pRenderPass               = m_pTonemappingRenderPass;
    TonemappingPipelineParams.pPipelineLayout           = m_pTonemappingPipelineLayout;
    
    m_pTonemappingPipeline = FGraphicsPipeline::Create(m_pDevice, TonemappingPipelineParams);
    assert(m_pTonemappingPipeline != nullptr);
    
    delete pVertex;
    delete pFragment;
}

void FRayTracer::CreateGlobalBuffers()
{
    // Camera
    FBufferParams CameraBufferParams;
    CameraBufferParams.Size             = sizeof(FCameraBuffer);
    CameraBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    CameraBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pCameraBuffer = FBuffer::Create(m_pDevice, CameraBufferParams, m_pDeviceAllocator);
    assert(m_pCameraBuffer != nullptr);
    SetDebugName(m_pDevice->GetDevice(), "Camera-Buffer", reinterpret_cast<uint64_t>(m_pCameraBuffer->GetBuffer()), VK_OBJECT_TYPE_BUFFER);
    
    // Random
    FBufferParams RandomBufferParams;
    RandomBufferParams.Size             = sizeof(FRandomBuffer);
    RandomBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    RandomBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pRandomBuffer = FBuffer::Create(m_pDevice, RandomBufferParams, m_pDeviceAllocator);
    assert(m_pRandomBuffer != nullptr);
    SetDebugName(m_pDevice->GetDevice(), "Random-Buffer", reinterpret_cast<uint64_t>(m_pRandomBuffer->GetBuffer()), VK_OBJECT_TYPE_BUFFER);
    
    // SceneBuffer
    FBufferParams SceneBufferParams;
    SceneBufferParams.Size             = sizeof(FSceneBuffer);
    SceneBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    SceneBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pSceneBuffer = FBuffer::Create(m_pDevice, SceneBufferParams, m_pDeviceAllocator);
    assert(m_pSceneBuffer != nullptr);
    SetDebugName(m_pDevice->GetDevice(), "Scene-Buffer", reinterpret_cast<uint64_t>(m_pSceneBuffer->GetBuffer()), VK_OBJECT_TYPE_BUFFER);
    
    // TonemappingBuffer
    FBufferParams TonemappingBufferParams;
    TonemappingBufferParams.Size             = sizeof(FTonemappingBuffer);
    TonemappingBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    TonemappingBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pTonemappingBuffer = FBuffer::Create(m_pDevice, TonemappingBufferParams, m_pDeviceAllocator);
    assert(m_pTonemappingBuffer != nullptr);
    SetDebugName(m_pDevice->GetDevice(), "Tonemapping-Buffer", reinterpret_cast<uint64_t>(m_pTonemappingBuffer->GetBuffer()), VK_OBJECT_TYPE_BUFFER);
    
    // QuadBuffer
    FBufferParams QuadBufferParams;
    QuadBufferParams.Size             = sizeof(FShaderQuad) * MAX_QUADS;
    QuadBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    QuadBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pQuadBuffer = FBuffer::Create(m_pDevice, QuadBufferParams, m_pDeviceAllocator);
    assert(m_pQuadBuffer != nullptr);
    SetDebugName(m_pDevice->GetDevice(), "Quad-Buffer", reinterpret_cast<uint64_t>(m_pQuadBuffer->GetBuffer()), VK_OBJECT_TYPE_BUFFER);
    
    // SphereBuffer
    FBufferParams SphereBufferParams;
    SphereBufferParams.Size             = sizeof(FShaderSphere) * MAX_SPHERES;
    SphereBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    SphereBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pSphereBuffer = FBuffer::Create(m_pDevice, SphereBufferParams, m_pDeviceAllocator);
    assert(m_pSphereBuffer != nullptr);
    SetDebugName(m_pDevice->GetDevice(), "Sphere-Buffer", reinterpret_cast<uint64_t>(m_pSphereBuffer->GetBuffer()), VK_OBJECT_TYPE_BUFFER);
    
    // VertexBuffer
    FBufferParams VertexBufferParams;
    VertexBufferParams.Size             = sizeof(FVertexPosOnly) * MAX_VERTICES;
    VertexBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    VertexBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pVertexBuffer = FBuffer::Create(m_pDevice, VertexBufferParams, m_pDeviceAllocator);
    assert(m_pVertexBuffer != nullptr);
    SetDebugName(m_pDevice->GetDevice(), "Vertex-Buffer", reinterpret_cast<uint64_t>(m_pVertexBuffer->GetBuffer()), VK_OBJECT_TYPE_BUFFER);
    
    // TriangleBuffer
    FBufferParams TriangleBufferParams;
    TriangleBufferParams.Size             = sizeof(FShaderTriangle) * MAX_TRIANGLES;
    TriangleBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    TriangleBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pTriangleBuffer = FBuffer::Create(m_pDevice, TriangleBufferParams, m_pDeviceAllocator);
    assert(m_pTriangleBuffer != nullptr);
    SetDebugName(m_pDevice->GetDevice(), "Triangle-Buffer", reinterpret_cast<uint64_t>(m_pTriangleBuffer->GetBuffer()), VK_OBJECT_TYPE_BUFFER);
    
    // TriangleMeshesBuffer
    FBufferParams MeshBufferParams;
    MeshBufferParams.Size             = sizeof(FShaderMesh) * MAX_TRIANGLEMESHES;
    MeshBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    MeshBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pMeshBuffer = FBuffer::Create(m_pDevice, MeshBufferParams, m_pDeviceAllocator);
    assert(m_pMeshBuffer != nullptr);
    SetDebugName(m_pDevice->GetDevice(), "TriangleMeshes-Buffer", reinterpret_cast<uint64_t>(m_pMeshBuffer->GetBuffer()), VK_OBJECT_TYPE_BUFFER);
    
    // MaterialBuffer
    FBufferParams MaterialBufferParams;
    MaterialBufferParams.Size             = sizeof(FShaderMaterial) * MAX_MATERIALS;
    MaterialBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    MaterialBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pMaterialBuffer = FBuffer::Create(m_pDevice, MaterialBufferParams, m_pDeviceAllocator);
    assert(m_pMaterialBuffer != nullptr);
    SetDebugName(m_pDevice->GetDevice(), "Material-Buffer", reinterpret_cast<uint64_t>(m_pMaterialBuffer->GetBuffer()), VK_OBJECT_TYPE_BUFFER);
    
    // BoundingBoxBuffer
    FBufferParams BoundingBoxBufferParams;
    BoundingBoxBufferParams.Size             = sizeof(FShaderBoundingBox) * MAX_BVH_NODES;
    BoundingBoxBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    BoundingBoxBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pBvhBuffer = FBuffer::Create(m_pDevice, BoundingBoxBufferParams, m_pDeviceAllocator);
    assert(m_pBvhBuffer != nullptr);
    SetDebugName(m_pDevice->GetDevice(), "BVH-Buffer", reinterpret_cast<uint64_t>(m_pBvhBuffer->GetBuffer()), VK_OBJECT_TYPE_BUFFER);
}

void FRayTracer::CreateDescriptorSet()
{
    m_pRayTracingDescriptorSet0 = FDescriptorSet::Create(m_pDevice, m_pDescriptorPool, m_pRayTracingDescriptorSetLayout);
    assert(m_pRayTracingDescriptorSet0 != nullptr);

    m_pRayTracingDescriptorSet0->BindStorageImage(m_pSceneTextureView0->GetImageView(), 0);
    m_pRayTracingDescriptorSet0->BindStorageImage(m_pSceneTextureView1->GetImageView(), 1);
    m_pRayTracingDescriptorSet0->BindCombinedImageSampler(m_pSkybox->GetTextureView()->GetImageView(), m_pSkyboxSampler->GetSampler(), 2);
    m_pRayTracingDescriptorSet0->BindUniformBuffer(m_pCameraBuffer->GetBuffer(), 3);
    m_pRayTracingDescriptorSet0->BindUniformBuffer(m_pRandomBuffer->GetBuffer(), 4);
    m_pRayTracingDescriptorSet0->BindUniformBuffer(m_pSceneBuffer->GetBuffer(), 5);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pQuadBuffer->GetBuffer(), 6);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pSphereBuffer->GetBuffer(), 7);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pMaterialBuffer->GetBuffer(), 8);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pVertexBuffer->GetBuffer(), 9);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pTriangleBuffer->GetBuffer(), 10);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pMeshBuffer->GetBuffer(), 11);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pBvhBuffer->GetBuffer(), 12);
    
    m_pRayTracingDescriptorSet1 = FDescriptorSet::Create(m_pDevice, m_pDescriptorPool, m_pRayTracingDescriptorSetLayout);
    assert(m_pRayTracingDescriptorSet0 != nullptr);

    m_pRayTracingDescriptorSet1->BindStorageImage(m_pSceneTextureView1->GetImageView(), 0);
    m_pRayTracingDescriptorSet1->BindStorageImage(m_pSceneTextureView0->GetImageView(), 1);
    m_pRayTracingDescriptorSet1->BindCombinedImageSampler(m_pSkybox->GetTextureView()->GetImageView(), m_pSkyboxSampler->GetSampler(), 2);
    m_pRayTracingDescriptorSet1->BindUniformBuffer(m_pCameraBuffer->GetBuffer(), 3);
    m_pRayTracingDescriptorSet1->BindUniformBuffer(m_pRandomBuffer->GetBuffer(), 4);
    m_pRayTracingDescriptorSet1->BindUniformBuffer(m_pSceneBuffer->GetBuffer(), 5);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pQuadBuffer->GetBuffer(), 6);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pSphereBuffer->GetBuffer(), 7);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pMaterialBuffer->GetBuffer(), 8);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pVertexBuffer->GetBuffer(), 9);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pTriangleBuffer->GetBuffer(), 10);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pMeshBuffer->GetBuffer(), 11);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pBvhBuffer->GetBuffer(), 12);
    
    m_pTonemappingDescriptorSet0 = FDescriptorSet::Create(m_pDevice, m_pDescriptorPool, m_pTonemappingDescriptorSetLayout);
    assert(m_pTonemappingDescriptorSet0 != nullptr);

    m_pTonemappingDescriptorSet0->BindCombinedImageSampler(m_pSceneTextureView0->GetImageView(), m_pTonemapSampler->GetSampler(), 0);
    m_pTonemappingDescriptorSet0->BindUniformBuffer(m_pTonemappingBuffer->GetBuffer(), 1);
    
    m_pTonemappingDescriptorSet1 = FDescriptorSet::Create(m_pDevice, m_pDescriptorPool, m_pTonemappingDescriptorSetLayout);
    assert(m_pTonemappingDescriptorSet1 != nullptr);

    m_pTonemappingDescriptorSet1->BindCombinedImageSampler(m_pSceneTextureView1->GetImageView(), m_pTonemapSampler->GetSampler(), 0);
    m_pTonemappingDescriptorSet1->BindUniformBuffer(m_pTonemappingBuffer->GetBuffer(), 1);
}

void FRayTracer::ReleaseDescriptorSets()
{
    SAFE_DELETE(m_pRayTracingDescriptorSet0);
    SAFE_DELETE(m_pRayTracingDescriptorSet1);
    SAFE_DELETE(m_pTonemappingDescriptorSet0);
    SAFE_DELETE(m_pTonemappingDescriptorSet1);
    SAFE_DELETE(m_pOutputTextureDescriptorSet);
}

void FRayTracer::CreateOrResizeSceneTexture(uint32_t Width, uint32_t Height)
{
    if (m_pSceneTexture0)
    {
        if ((m_pSceneTexture0->GetWidth() == Width && m_pSceneTexture0->GetHeight() == Height) || Width == 0 || Height == 0)
        {
            return;
        }

        m_pDevice->WaitForIdle();

        SAFE_DELETE(m_pSceneTexture0);
        SAFE_DELETE(m_pSceneTextureView0);
        SAFE_DELETE(m_pSceneTexture1);
        SAFE_DELETE(m_pSceneTextureView1);
        SAFE_DELETE(m_pOutputTexture);
        SAFE_DELETE(m_pOutputTextureView);
        SAFE_DELETE(m_pTonemappingFramebuffer);
        
        ReleaseDescriptorSets();
    }

    // Create texture for the viewport
    FTextureParams TextureParams = {};
    TextureParams.Format        = VK_FORMAT_R32G32B32A32_SFLOAT;
    TextureParams.ImageType     = VK_IMAGE_TYPE_2D;
    TextureParams.Width         = m_ViewportWidth  = Width;
    TextureParams.Height        = m_ViewportHeight = Height;
    TextureParams.Usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    TextureParams.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Scene texture frame 0
    m_pSceneTexture0 = FTexture::Create(m_pDevice, TextureParams);
    assert(m_pSceneTexture0 != nullptr);
    SetDebugName(m_pDevice->GetDevice(), "SceneTexture0", reinterpret_cast<uint64_t>(m_pSceneTexture0->GetImage()), VK_OBJECT_TYPE_IMAGE);

    {
        FTextureViewParams TextureViewParams = {};
        TextureViewParams.pTexture = m_pSceneTexture0;

        m_pSceneTextureView0 = FTextureView::Create(m_pDevice, TextureViewParams);
        assert(m_pSceneTextureView0 != nullptr);
        SetDebugName(m_pDevice->GetDevice(), "SceneTextureView0", reinterpret_cast<uint64_t>(m_pSceneTextureView0->GetImageView()), VK_OBJECT_TYPE_IMAGE_VIEW);
    }

    // Scene texture frame 1
    m_pSceneTexture1 = FTexture::Create(m_pDevice, TextureParams);
    assert(m_pSceneTexture1 != nullptr);
    SetDebugName(m_pDevice->GetDevice(), "SceneTexture1", reinterpret_cast<uint64_t>(m_pSceneTexture1->GetImage()), VK_OBJECT_TYPE_IMAGE);
    
    {
        FTextureViewParams TextureViewParams = {};
        TextureViewParams.pTexture = m_pSceneTexture1;
        
        m_pSceneTextureView1 = FTextureView::Create(m_pDevice, TextureViewParams);
        assert(m_pSceneTextureView1 != nullptr);
        SetDebugName(m_pDevice->GetDevice(), "SceneTextureView1", reinterpret_cast<uint64_t>(m_pSceneTextureView1->GetImageView()), VK_OBJECT_TYPE_IMAGE_VIEW);
    }

    // Create texture for the viewport
    FTextureParams OutputTextureParams = {};
    OutputTextureParams.Format        = VK_FORMAT_R8G8B8A8_UNORM;
    OutputTextureParams.ImageType     = VK_IMAGE_TYPE_2D;
    OutputTextureParams.Width         = m_ViewportWidth  = Width;
    OutputTextureParams.Height        = m_ViewportHeight = Height;
    OutputTextureParams.Usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    OutputTextureParams.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    m_pOutputTexture = FTexture::Create(m_pDevice, OutputTextureParams);
    assert(m_pOutputTexture != nullptr);
    SetDebugName(m_pDevice->GetDevice(), "OutputTexture", reinterpret_cast<uint64_t>(m_pOutputTexture->GetImage()), VK_OBJECT_TYPE_IMAGE);
    
    {
        FTextureViewParams TextureViewParams = {};
        TextureViewParams.pTexture = m_pOutputTexture;
        
        m_pOutputTextureView = FTextureView::Create(m_pDevice, TextureViewParams);
        assert(m_pOutputTextureView != nullptr);
        SetDebugName(m_pDevice->GetDevice(), "OutputTextureView", reinterpret_cast<uint64_t>(m_pOutputTextureView->GetImageView()), VK_OBJECT_TYPE_IMAGE_VIEW);
    }
    
    // Create Framebuffer for the tonemap stage
    VkImageView ImageView = m_pOutputTextureView->GetImageView();
    FFramebufferParams FramebufferParams = {};
    FramebufferParams.AttachMentCount = 1;
    FramebufferParams.Width           = m_ViewportWidth;
    FramebufferParams.Height          = m_ViewportHeight;
    FramebufferParams.pRenderPass     = m_pTonemappingRenderPass;
    FramebufferParams.pAttachMents    = &ImageView;

    m_pTonemappingFramebuffer = FFramebuffer::Create(m_pDevice, FramebufferParams);
    
    // UI DescriptorSet
    m_pOutputTextureDescriptorSet = GUI::AllocateTextureID(m_pOutputTextureView);
    assert(m_pOutputTextureDescriptorSet != nullptr);

    // Descriptor set for when tracing
    CreateDescriptorSet();

    // When we have resized we need to clear the image as well
    m_bResetImage = true;
}

void FRayTracer::ReloadShader()
{
    static bool bIsCompiling = false;

    if (!bIsCompiling)
    {
        bIsCompiling = true;

        std::async(std::launch::async, [this]()
        {
            // Compile the shaders
            auto Result = std::system(SHADER_SCRIPT_PATH);
            if (Result != 0)
            {
                std::cout << "FAILED to Compile Shaders\n";
                bIsCompiling = false;
                return false;
            }

            // Upload the new shaders
            std::cout << "Compiled Shaders Successfully\n";

            // Create shader and pipeline
            FShaderModule* pComputeShader = FShaderModule::CreateFromFile(m_pDevice, "main", RESOURCE_PATH"/shaders/raytracer.spv");
            if (!pComputeShader)
            {
                std::cout << "FAILED to create ComputeShader\n";
                bIsCompiling = false;
                return false;
            }

            FComputePipelineStateParams pipelineParams = {};
            pipelineParams.pShader         = pComputeShader;
            pipelineParams.pPipelineLayout = m_pRayTracingPipelineLayout;

            FComputePipeline* pComputePipeline  = FComputePipeline::Create(m_pDevice, pipelineParams);
            if (!pComputePipeline)
            {
                std::cout << "FAILED to create ComputePipeline\n";
                SAFE_DELETE(pComputeShader);
                bIsCompiling = false;
                return false;
            }

            m_pDevice->WaitForIdle();
            pComputePipeline = m_pRayTracingPipeline.exchange(pComputePipeline);

            SAFE_DELETE(pComputeShader);
            SAFE_DELETE(pComputePipeline);

            m_bResetImage = true;
            bIsCompiling  = false;
            return true;
        });
    }
}

void FRayTracer::UpdateGlobalBuffers(FCommandBuffer* pCommandBuffer)
{
    if (!m_pScene->m_Quads.empty())
    {
        assert(sizeof(FShaderQuad) * m_pScene->m_Quads.size() < m_pQuadBuffer->GetSize());
        pCommandBuffer->UpdateBuffer(m_pQuadBuffer, 0, sizeof(FShaderQuad) * m_pScene->m_Quads.size(), m_pScene->m_Quads.data());
    }
    
    if (!m_pScene->m_Spheres.empty())
    {
        assert(sizeof(FShaderSphere) * m_pScene->m_Spheres.size() < m_pSphereBuffer->GetSize());
        pCommandBuffer->UpdateBuffer(m_pSphereBuffer, 0, sizeof(FShaderSphere) * m_pScene->m_Spheres.size(), m_pScene->m_Spheres.data());
    }
    
    if (m_pScene->m_pVertexBuffer)
    {
        VkBufferCopy BufferCopy;
        BufferCopy.size      = m_pScene->m_pVertexBuffer->GetSize();
        BufferCopy.dstOffset = 0;
        BufferCopy.srcOffset = 0;
        
        pCommandBuffer->CopyBuffer(m_pScene->m_pVertexBuffer->GetBuffer(), m_pVertexBuffer->GetBuffer(), 1, &BufferCopy);
    }
    
    if (m_pScene->m_pTriangleBuffer)
    {
        VkBufferCopy BufferCopy;
        BufferCopy.size      = m_pScene->m_pTriangleBuffer->GetSize();
        BufferCopy.dstOffset = 0;
        BufferCopy.srcOffset = 0;
        
        pCommandBuffer->CopyBuffer(m_pScene->m_pTriangleBuffer->GetBuffer(), m_pTriangleBuffer->GetBuffer(), 1, &BufferCopy);
    }
    
    if (!m_pScene->m_Meshes.empty())
    {
        assert(sizeof(FShaderMesh) * m_pScene->m_Meshes.size() < m_pMeshBuffer->GetSize());
        pCommandBuffer->UpdateBuffer(m_pMeshBuffer, 0, sizeof(FShaderMesh) * m_pScene->m_Meshes.size(), m_pScene->m_Meshes.data());
    }
    
    if (!m_pScene->m_Materials.empty())
    {
        assert(sizeof(FShaderMaterial) * m_pScene->m_Materials.size() < m_pMaterialBuffer->GetSize());
        pCommandBuffer->UpdateBuffer(m_pMaterialBuffer, 0, sizeof(FShaderMaterial) * m_pScene->m_Materials.size(), m_pScene->m_Materials.data());
    }
    
    if (m_pScene->m_pBoundingBoxBuffer)
    {
        VkBufferCopy BufferCopy;
        BufferCopy.size      = m_pScene->m_pBoundingBoxBuffer->GetSize();
        BufferCopy.dstOffset = 0;
        BufferCopy.srcOffset = 0;
        
        pCommandBuffer->CopyBuffer(m_pScene->m_pBoundingBoxBuffer->GetBuffer(), m_pBvhBuffer->GetBuffer(), 1, &BufferCopy);
    }
}
