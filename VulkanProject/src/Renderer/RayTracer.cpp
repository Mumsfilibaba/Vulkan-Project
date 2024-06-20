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
    , m_pPlaneBuffer(nullptr)
    , m_pQuadBuffer(nullptr)
    , m_pTriangleBuffer(nullptr)
    , m_pTriangleMeshesBuffer(nullptr)
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
        FSamplerParams samplerParams = {};
        samplerParams.magFilter     = VK_FILTER_LINEAR;
        samplerParams.minFilter     = VK_FILTER_LINEAR;
        samplerParams.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerParams.addressModeU  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerParams.addressModeV  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerParams.addressModeW  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerParams.minLod        = 0;
        samplerParams.maxLod        = 1000;
        samplerParams.maxAnisotropy = 1.0f;
        
        m_pSkyboxSampler = FSampler::Create(pDevice, samplerParams);
        assert(m_pSkyboxSampler != nullptr);
    }
    
    // Tonemap Sampler
    {
        FSamplerParams samplerParams = {};
        samplerParams.magFilter     = VK_FILTER_NEAREST;
        samplerParams.minFilter     = VK_FILTER_NEAREST;
        samplerParams.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerParams.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerParams.addressModeV  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerParams.addressModeW  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerParams.minLod        = 0;
        samplerParams.maxLod        = 1000;
        samplerParams.maxAnisotropy = 1.0f;
        
        m_pTonemapSampler = FSampler::Create(pDevice, samplerParams);
        assert(m_pTonemapSampler != nullptr);
    }
    
    // Create scene
    m_pScene = new FSphereScene();
    m_pScene->Initialize();

    // RenderPasses
    CreateRayTracingResources();
    CreateTonemappingResources();
    
    // Create all buffers
    CreateGlobalBuffers();
    
    // Create DescriptorPool
    FDescriptorPoolParams poolParams;
    poolParams.NumUniformBuffers        = 32;
    poolParams.NumStorageImages         = 32;
    poolParams.NumStorageBuffers        = 32;
    poolParams.NumCombinedImageSamplers = 32;
    poolParams.MaxSets                  = 4;
    
    m_pDescriptorPool = FDescriptorPool::Create(m_pDevice, poolParams);
    assert(m_pDescriptorPool != nullptr);

    // Create the scene texture
    m_ViewportWidth  = 0;
    m_ViewportHeight = 0;
    CreateOrResizeSceneTexture(1280, 720);

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

    // Timestamp queries
    FQueryParams queryParams;
    queryParams.queryType  = VK_QUERY_TYPE_TIMESTAMP;
    queryParams.queryCount = 2;
    
    m_TimestampQueries.resize(imageCount);
    for (size_t i = 0; i < m_TimestampQueries.size(); i++)
    {
        FQuery* pQuery = FQuery::Create(m_pDevice, queryParams);
        pQuery->Reset();
        
        m_TimestampQueries[i] = pQuery;
    }
    
    // Allocator for GPU memory
    m_pDeviceAllocator = new FDeviceMemoryAllocator(m_pDevice->GetDevice(), m_pDevice->GetPhysicalDevice());
}

void FRayTracer::Tick(float deltaTime)
{
    constexpr float CameraSpeed = 1.5f;
    m_LastCPUTime = deltaTime * 1000.0f; // deltaTime is in seconds

    // Update scene image
    CreateOrResizeSceneTexture(m_ViewportWidth, m_ViewportHeight);

    // Camera Movement
    glm::vec3 translation(0.0f);
    if (FInput::IsKeyDown(GLFW_KEY_W))
    {
        translation.z = CameraSpeed * deltaTime;
    }
    else if (FInput::IsKeyDown(GLFW_KEY_S))
    {
        translation.z = -CameraSpeed * deltaTime;
    }

    if (FInput::IsKeyDown(GLFW_KEY_A))
    {
        translation.x = CameraSpeed * deltaTime;
    }
    else if (FInput::IsKeyDown(GLFW_KEY_D))
    {
        translation.x = -CameraSpeed * deltaTime;
    }

    m_pScene->m_Camera.Move(translation);

    // Camera rotation
    constexpr float CameraRotationSpeed = glm::pi<float>() / 2;

    glm::vec3 rotation(0.0f);
    if (FInput::IsKeyDown(GLFW_KEY_LEFT))
    {
        rotation.y = -CameraRotationSpeed * deltaTime;
    }
    else if (FInput::IsKeyDown(GLFW_KEY_RIGHT))
    {
        rotation.y = CameraRotationSpeed * deltaTime;
    }

    if (FInput::IsKeyDown(GLFW_KEY_UP))
    {
        rotation.x = -CameraRotationSpeed * deltaTime;
    }
    else if (FInput::IsKeyDown(GLFW_KEY_DOWN))
    {
        rotation.x = CameraRotationSpeed * deltaTime;
    }

    m_pScene->m_Camera.Rotate(rotation);

    // Check if we moved and then we reset the image
    if (glm::length(rotation) > 0.0f || glm::length(translation) > 0.0f)
    {
        m_bResetImage = true;
    }

    // Reload shaders
    if (FInput::IsKeyDown(GLFW_KEY_R))
    {
        ReloadShader();
    }

    // Update
    m_pScene->m_Camera.Update(m_pScene->m_Settings.FieldOfView, m_pSceneTexture0->GetWidth(), m_pSceneTexture0->GetHeight(), 0.1f, 100.0f);

    // Draw
    uint32_t frameIndex = m_pSwapchain->GetCurrentBackBufferIndex();
    FQuery*         pCurrentTimestampQuery = m_TimestampQueries[frameIndex];
    FCommandBuffer* pCurrentCommandBuffer  = m_CommandBuffers[frameIndex];

    // Reset CommandBuffer
    pCurrentCommandBuffer->Reset();

    // Prepare timestamps
    constexpr uint32_t timestampCount = 2;
    uint64_t timestamps[timestampCount];
    ZERO_MEMORY(timestamps, sizeof(uint64_t) * timestampCount);

    pCurrentTimestampQuery->GetData(0, 2, sizeof(uint64_t) * timestampCount, &timestamps, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
    pCurrentTimestampQuery->Reset();

    const double timestampPeriod = double(m_pDevice->GetTimestampPeriod());
    const double gpuTiming   = (double(timestamps[1]) - double(timestamps[0])) * timestampPeriod;
    const double gpuTimingMS = gpuTiming / 1000000.0;
    m_LastGPUTime = static_cast<float>(gpuTimingMS);

    // Begin CommandBuffer
    pCurrentCommandBuffer->Begin();
    pCurrentCommandBuffer->WriteTimestamp(pCurrentTimestampQuery, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);

    pCurrentCommandBuffer->TransitionImage(m_pSceneTexture0->GetImage(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
    pCurrentCommandBuffer->TransitionImage(m_pSceneTexture1->GetImage(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

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

        pCurrentCommandBuffer->ClearColorImage(m_pSceneTexture0->GetImage(), VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &subresourceRange);
        pCurrentCommandBuffer->ClearColorImage(m_pSceneTexture1->GetImage(), VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &subresourceRange);

        m_bResetImage = false;
        m_FrameIndex  = 0;
    }
    else
    {
        m_FrameIndex++;
    }

    // Update CameraBuffer
    FCameraBuffer cameraBuffer = {};
    cameraBuffer.Projection         = m_pScene->m_Camera.GetProjectionMatrix();
    cameraBuffer.View               = m_pScene->m_Camera.GetViewMatrix();
    cameraBuffer.Position           = glm::vec4(m_pScene->m_Camera.GetPosition(), 0.0f);
    cameraBuffer.Forward            = glm::vec4(m_pScene->m_Camera.GetForward(), 0.0f);
    cameraBuffer.FieldOfViewDegrees = Math::ToDegrees(m_pScene->m_Camera.GetFieldOfView());
    
    pCurrentCommandBuffer->UpdateBuffer(m_pCameraBuffer, 0, sizeof(FCameraBuffer), &cameraBuffer);

    // Update RandomBuffer
    constexpr uint32_t maxSamples = 16;

    FRandomBuffer randomBuffer = {};
    randomBuffer.FrameIndex  = m_FrameIndex;
    randomBuffer.HaltonIndex = m_FrameIndex % maxSamples;

    pCurrentCommandBuffer->UpdateBuffer(m_pRandomBuffer, 0, sizeof(FRandomBuffer), &randomBuffer);

    // Update Tonemapping Settings
    FTonemappingBuffer tonemappingBuffer = {};
    tonemappingBuffer.Exposure = m_pScene->m_Settings.Exposure;
    
    pCurrentCommandBuffer->UpdateBuffer(m_pTonemappingBuffer, 0, sizeof(FTonemappingBuffer), &tonemappingBuffer);
    
    // Update Scene
    FSceneBuffer sceneBuffer = {};
    sceneBuffer.NumQuads          = m_pScene->m_Quads.size();
    sceneBuffer.NumSpheres        = m_pScene->m_Spheres.size();
    sceneBuffer.NumPlanes         = m_pScene->m_Planes.size();
    sceneBuffer.NumMaterials      = m_pScene->m_Materials.size();
    sceneBuffer.NumTriangleMeshes = m_pScene->m_TriangleMeshes.size();
    sceneBuffer.BackgroundType    = m_pScene->m_Settings.BackgroundType;
    sceneBuffer.NumBounces        = m_pScene->m_Settings.NumBounces;

    pCurrentCommandBuffer->UpdateBuffer(m_pSceneBuffer, 0, sizeof(FSceneBuffer), &sceneBuffer);
    
    if (!m_pScene->m_Quads.empty())
    {
        pCurrentCommandBuffer->UpdateBuffer(m_pQuadBuffer, 0, sizeof(FQuad) * m_pScene->m_Quads.size(), m_pScene->m_Quads.data());
    }
    if (!m_pScene->m_Spheres.empty())
    {
        pCurrentCommandBuffer->UpdateBuffer(m_pSphereBuffer, 0, sizeof(FSphere) * m_pScene->m_Spheres.size(), m_pScene->m_Spheres.data());
    }
    if (!m_pScene->m_Planes.empty())
    {
        pCurrentCommandBuffer->UpdateBuffer(m_pPlaneBuffer, 0, sizeof(FPlane) * m_pScene->m_Planes.size(), m_pScene->m_Planes.data());
    }
    if (!m_pScene->m_Vertices.empty())
    {
        pCurrentCommandBuffer->UpdateBuffer(m_pVertexBuffer, 0, sizeof(FVertexRT) * m_pScene->m_Vertices.size(), m_pScene->m_Vertices.data());
    }
    if (!m_pScene->m_Triangles.empty())
    {
        pCurrentCommandBuffer->UpdateBuffer(m_pTriangleBuffer, 0, sizeof(FTriangle) * m_pScene->m_Triangles.size(), m_pScene->m_Triangles.data());
    }
    if (!m_pScene->m_TriangleMeshes.empty())
    {
        pCurrentCommandBuffer->UpdateBuffer(m_pTriangleMeshesBuffer, 0, sizeof(FTriangleMesh) * m_pScene->m_TriangleMeshes.size(), m_pScene->m_TriangleMeshes.data());
    }
    if (!m_pScene->m_Materials.empty())
    {
        pCurrentCommandBuffer->UpdateBuffer(m_pMaterialBuffer, 0, sizeof(FMaterial) * m_pScene->m_Materials.size(), m_pScene->m_Materials.data());
    }

    // Bind pipeline and descriptorSet
    pCurrentCommandBuffer->BindComputePipelineState(m_pRayTracingPipeline.load());
    
    const uint64_t frame = (m_FrameIndex % 2);
    if (frame == 0)
    {
        pCurrentCommandBuffer->BindComputeDescriptorSet(m_pRayTracingPipelineLayout, m_pRayTracingDescriptorSet0);
    }
    else
    {
        pCurrentCommandBuffer->BindComputeDescriptorSet(m_pRayTracingPipelineLayout, m_pRayTracingDescriptorSet1);
    }

    // Dispatch RayTracing
    const uint32_t Threads = 16;
    VkExtent2D dispatchSize = { Math::AlignUp(m_pSceneTexture0->GetWidth(), Threads) / Threads, Math::AlignUp(m_pSceneTexture0->GetHeight(), Threads) / Threads };
    pCurrentCommandBuffer->Dispatch(dispatchSize.width, dispatchSize.height, 1);

    pCurrentCommandBuffer->TransitionImage(m_pSceneTexture0->GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    pCurrentCommandBuffer->TransitionImage(m_pSceneTexture1->GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Begin renderpass
    VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    pCurrentCommandBuffer->BeginRenderPass(m_pTonemappingRenderPass, m_pTonemappingFramebuffer, &clearColor, 1);
    
    // Set viewport
    VkViewport viewport = { 0.0f, 0.0f, float(m_ViewportWidth), float(m_ViewportHeight), 0.0f, 1.0f };
    pCurrentCommandBuffer->SetViewport(viewport);
    
    VkRect2D scissor = { { 0, 0}, { m_ViewportWidth, m_ViewportHeight } };
    pCurrentCommandBuffer->SetScissorRect(scissor);
    
    // Bind pipeline
    pCurrentCommandBuffer->BindGraphicsPipelineState(m_pTonemappingPipeline);

    // Perform tonemapping
    if (frame == 0)
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
        
        ImGui::Text("Current Resolution: %dx%d", m_ViewportWidth, m_ViewportHeight);

        ImGui::NewLine();

        ImGui::Text("Image:");
        ImGui::Separator();

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
            static const char* scenes[] =
            {
                "Spheres",
                "CornellBox",
                "Triangles"
            };

            static int currentScene = 0;
            static int prevScene    = 0;
            ImGui::Combo("Current Scene", &currentScene, scenes, IM_ARRAYSIZE(scenes));

            if (prevScene != currentScene)
            {
                // Change to Sphere-scene
                if (currentScene == 0)
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = new FSphereScene();
                    m_pScene->Initialize();
                    m_bResetImage = true;
                }

                // Change to CornellBox-scene
                if (currentScene == 1)
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = new FCornellBoxScene();
                    m_pScene->Initialize();
                    m_bResetImage = true;
                }
                
                // Change to Triangles-scene
                if (currentScene == 2)
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = new FModelScene();
                    m_pScene->Initialize();
                    m_bResetImage = true;
                }

                prevScene = currentScene;
            }
            
            // Background
            static const char* background[] =
            {
                "None",
                "Gradient",
                "Skybox",
            };
            
            int currentBG = m_pScene->m_Settings.BackgroundType;
            static int prevBG = currentBG;
            ImGui::Combo("Background", &currentBG, background, IM_ARRAYSIZE(background));
            
            if (prevBG != currentBG)
            {
                if (currentBG == 0)
                {
                    m_pScene->m_Settings.BackgroundType = BACKGROUND_TYPE_NONE;
                }
                else if (currentBG == 1)
                {
                    m_pScene->m_Settings.BackgroundType = BACKGROUND_TYPE_GRADIENT;
                }
                else if (currentBG == 2)
                {
                    m_pScene->m_Settings.BackgroundType = BACKGROUND_TYPE_SKYBOX;
                }
                
                m_bResetImage = true;
                prevBG = currentBG;
            }

            float exposure = m_pScene->m_Settings.Exposure;
            if (ImGui::DragFloat("Exposure", &exposure, 0.01f, 0.0f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
            {
                m_pScene->m_Settings.Exposure = exposure;
                m_bResetImage = true;
            }
            
            float fieldOfView = m_pScene->m_Settings.FieldOfView;
            if (ImGui::DragFloat("FieldOfView", &fieldOfView, 0.1f, 30.0f, 120.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
            {
                m_pScene->m_Settings.FieldOfView = fieldOfView;
                m_bResetImage = true;
            }
            
            int numBounces = m_pScene->m_Settings.NumBounces;
            if (ImGui::DragInt("Num Bounces", &numBounces, 1, 1, 1024, "%d", ImGuiSliderFlags_AlwaysClamp))
            {
                m_pScene->m_Settings.NumBounces = numBounces;
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

        uint32_t imguiID = 0;
        {
            uint32_t index = 1;
            for (FSphere& sphere : m_pScene->m_Spheres)
            {
                ImGui::PushID(imguiID++);

                ImGui::Text("Sphere %d", index++);
                if (ImGui::DragFloat3("Position", glm::value_ptr(sphere.Position), 0.1f))
                {
                    m_bResetImage = true;
                }
                if (ImGui::DragFloat("Radius", &sphere.Radius, 0.01f))
                {
                    m_bResetImage = true;
                }

                ImGui::PopID();
                ImGui::Separator();
            }
        }

        {
            uint32_t index = 1;
            for (FQuad& quad : m_pScene->m_Quads)
            {
                ImGui::PushID(imguiID++);

                ImGui::Text("Quad %d", index++);
                if (ImGui::DragFloat3("Position", glm::value_ptr(quad.Position), 0.1f))
                {
                    m_bResetImage = true;
                }
                if (ImGui::DragFloat3("Edge0", glm::value_ptr(quad.Edge0), 0.1f))
                {
                    m_bResetImage = true;
                }
                if (ImGui::DragFloat3("Edge1", glm::value_ptr(quad.Edge1), 0.1f))
                {
                    m_bResetImage = true;
                }

                ImGui::PopID();
                ImGui::Separator();
            }
        }

        {
            uint32_t index = 1;
            for (FTriangleMesh& mesh : m_pScene->m_TriangleMeshes)
            {
                ImGui::PushID(imguiID++);

                ImGui::Text("TriangleMesh %d", index++);
                
                ImGui::InputInt("Num Triangles", reinterpret_cast<int*>(&mesh.NumTriangles), 1, 100, ImGuiInputTextFlags_ReadOnly);
                ImGui::InputFloat3("Bounds Min", glm::value_ptr(mesh.BoxMin), "%0.3f", ImGuiInputTextFlags_ReadOnly);
                ImGui::InputFloat3("Bounds Max", glm::value_ptr(mesh.BoxMax), "%0.3f", ImGuiInputTextFlags_ReadOnly);
                
                ImGui::PopID();
                ImGui::Separator();
            }
        }

        ImGui::NewLine();

        ImGui::Text("Materials:");
        ImGui::Separator();

        {
            static const char* materialTypes[] =
            {
                "None",
                "Lambertian",
                "Metal",
                "Emissive",
                "Dielectric",
            };

            uint32_t index = 1;
            for (FMaterial& material : m_pScene->m_Materials)
            {
                ImGui::PushID(imguiID++);
                ImGui::Text("Material %d", index++);
            
                // if (ImGui::ColorEdit3("Albedo", glm::value_ptr(material.Albedo)))
                if (ImGui::InputFloat3("AlbedoColor", glm::value_ptr(material.AlbedoColor)))
                {
                    m_bResetImage = true;
                }
                // if (ImGui::ColorEdit3("Emissive", glm::value_ptr(material.Emissive)))
                if (ImGui::InputFloat3("EmissiveColor", glm::value_ptr(material.EmissiveColor)))
                {
                    m_bResetImage = true;
                }
                // if (ImGui::ColorEdit3("Emissive", glm::value_ptr(material.Emissive)))
                if (ImGui::InputFloat3("SpecularColor", glm::value_ptr(material.SpecularColor)))
                {
                    m_bResetImage = true;
                }
                if (ImGui::DragFloat("SpecularFactor", &material.SpecularFactor, 0.01f, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
                {
                    m_bResetImage = true;
                }
                if (ImGui::DragFloat("Roughness", &material.Roughness, 0.01f, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
                {
                    m_bResetImage = true;
                }
                if (ImGui::DragFloat("RefractionIndex", &material.RefractionIndex, 0.01f, 0.0f, 10.0f, "%.2f"))
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

    m_ViewportWidth  = ImGui::GetContentRegionAvail().x;
    m_ViewportHeight = ImGui::GetContentRegionAvail().y;

    if (m_pOutputTexture)
    {
        ImGui::Image(m_pOutputTextureDescriptorSet, { (float)m_pOutputTexture->GetWidth(), (float)m_pOutputTexture->GetHeight() });
    }
    
    ImGui::End();
}

void FRayTracer::Release()
{
    FTextureResource::ReleaseLoader();
    
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

    ReleaseDescriptorSets();

    SAFE_DELETE(m_pCameraBuffer);
    SAFE_DELETE(m_pRandomBuffer);
    SAFE_DELETE(m_pSceneBuffer);
    SAFE_DELETE(m_pTonemappingBuffer);
    SAFE_DELETE(m_pQuadBuffer);
    SAFE_DELETE(m_pSphereBuffer);
    SAFE_DELETE(m_pVertexBuffer);
    SAFE_DELETE(m_pTriangleBuffer);
    SAFE_DELETE(m_pTriangleMeshesBuffer);
    SAFE_DELETE(m_pPlaneBuffer);
    SAFE_DELETE(m_pMaterialBuffer);
    
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

void FRayTracer::OnWindowResize(uint32_t width, uint32_t height)
{
}

void FRayTracer::CreateRayTracingResources()
{
    // Create RayTracing DescriptorSetLayout
    constexpr uint32_t numRayTracingBindings = 13;
    VkDescriptorSetLayoutBinding rayTracingBindings[numRayTracingBindings];
    
    // Output Image
    rayTracingBindings[0].binding            = 0;
    rayTracingBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    rayTracingBindings[0].descriptorCount    = 1;
    rayTracingBindings[0].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    rayTracingBindings[0].pImmutableSamplers = nullptr;

    // Accumulation image
    rayTracingBindings[1].binding            = 1;
    rayTracingBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    rayTracingBindings[1].descriptorCount    = 1;
    rayTracingBindings[1].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    rayTracingBindings[1].pImmutableSamplers = nullptr;
    
    // Skybox
    rayTracingBindings[2].binding            = 2;
    rayTracingBindings[2].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    rayTracingBindings[2].descriptorCount    = 1;
    rayTracingBindings[2].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    rayTracingBindings[2].pImmutableSamplers = nullptr;
    
    // Camera Buffer
    rayTracingBindings[3].binding            = 3;
    rayTracingBindings[3].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    rayTracingBindings[3].descriptorCount    = 1;
    rayTracingBindings[3].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    rayTracingBindings[3].pImmutableSamplers = nullptr;
    
    // Random Buffer
    rayTracingBindings[4].binding            = 4;
    rayTracingBindings[4].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    rayTracingBindings[4].descriptorCount    = 1;
    rayTracingBindings[4].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    rayTracingBindings[4].pImmutableSamplers = nullptr;

    // Scene Buffer
    rayTracingBindings[5].binding            = 5;
    rayTracingBindings[5].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    rayTracingBindings[5].descriptorCount    = 1;
    rayTracingBindings[5].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    rayTracingBindings[5].pImmutableSamplers = nullptr;

    // Quads Buffer
    rayTracingBindings[6].binding            = 6;
    rayTracingBindings[6].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    rayTracingBindings[6].descriptorCount    = 1;
    rayTracingBindings[6].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    rayTracingBindings[6].pImmutableSamplers = nullptr;

    // Spheres Buffer
    rayTracingBindings[7].binding            = 7;
    rayTracingBindings[7].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    rayTracingBindings[7].descriptorCount    = 1;
    rayTracingBindings[7].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    rayTracingBindings[7].pImmutableSamplers = nullptr;

    // Planes Buffer
    rayTracingBindings[8].binding            = 8;
    rayTracingBindings[8].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    rayTracingBindings[8].descriptorCount    = 1;
    rayTracingBindings[8].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    rayTracingBindings[8].pImmutableSamplers = nullptr;
    
    // Materials Buffer
    rayTracingBindings[9].binding            = 9;
    rayTracingBindings[9].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    rayTracingBindings[9].descriptorCount    = 1;
    rayTracingBindings[9].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    rayTracingBindings[9].pImmutableSamplers = nullptr;

    // Vertex Buffer
    rayTracingBindings[10].binding            = 10;
    rayTracingBindings[10].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    rayTracingBindings[10].descriptorCount    = 1;
    rayTracingBindings[10].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    rayTracingBindings[10].pImmutableSamplers = nullptr;
    
    // Triangles Buffer
    rayTracingBindings[11].binding            = 11;
    rayTracingBindings[11].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    rayTracingBindings[11].descriptorCount    = 1;
    rayTracingBindings[11].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    rayTracingBindings[11].pImmutableSamplers = nullptr;
    
    // TriangleMeshes Buffer
    rayTracingBindings[12].binding            = 12;
    rayTracingBindings[12].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    rayTracingBindings[12].descriptorCount    = 1;
    rayTracingBindings[12].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    rayTracingBindings[12].pImmutableSamplers = nullptr;

    FDescriptorSetLayoutParams rayTracingDescriptorSetLayoutParams;
    rayTracingDescriptorSetLayoutParams.pBindings   = rayTracingBindings;
    rayTracingDescriptorSetLayoutParams.numBindings = numRayTracingBindings;

    m_pRayTracingDescriptorSetLayout = FDescriptorSetLayout::Create(m_pDevice, rayTracingDescriptorSetLayoutParams);
    assert(m_pRayTracingDescriptorSetLayout != nullptr);

    // Create RayTracing PipelineLayout
    FPipelineLayoutParams rayTracingPipelineLayoutParams;
    rayTracingPipelineLayoutParams.ppLayouts  = &m_pRayTracingDescriptorSetLayout;
    rayTracingPipelineLayoutParams.numLayouts = 1;

    m_pRayTracingPipelineLayout = FPipelineLayout::Create(m_pDevice, rayTracingPipelineLayoutParams);
    assert(m_pRayTracingPipelineLayout != nullptr);

    // Create RayTracing shader and pipeline
    FShaderModule* pComputeShader = FShaderModule::CreateFromFile(m_pDevice, "main", RESOURCE_PATH"/shaders/raytracer.spv");
    assert(pComputeShader != nullptr);
    
    FComputePipelineStateParams pipelineParams = {};
    pipelineParams.pShader         = pComputeShader;
    pipelineParams.pPipelineLayout = m_pRayTracingPipelineLayout;
    
    m_pRayTracingPipeline = FComputePipeline::Create(m_pDevice, pipelineParams);
    assert(m_pRayTracingPipeline != nullptr);

    delete pComputeShader;
}

void FRayTracer::CreateTonemappingResources()
{
    // Create Tonemapping DescriptorSetLayout
    constexpr uint32_t numTonemappingBindings = 2;
    VkDescriptorSetLayoutBinding tonemappingBindings[numTonemappingBindings];
    
    // Output Image
    tonemappingBindings[0].binding            = 0;
    tonemappingBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    tonemappingBindings[0].descriptorCount    = 1;
    tonemappingBindings[0].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    tonemappingBindings[0].pImmutableSamplers = nullptr;

    // Accumulation image
    tonemappingBindings[1].binding            = 1;
    tonemappingBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    tonemappingBindings[1].descriptorCount    = 1;
    tonemappingBindings[1].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    tonemappingBindings[1].pImmutableSamplers = nullptr;
    
    FDescriptorSetLayoutParams tonemappingDescriptorSetLayoutParams;
    tonemappingDescriptorSetLayoutParams.pBindings   = tonemappingBindings;
    tonemappingDescriptorSetLayoutParams.numBindings = numTonemappingBindings;

    m_pTonemappingDescriptorSetLayout = FDescriptorSetLayout::Create(m_pDevice, tonemappingDescriptorSetLayoutParams);
    assert(m_pTonemappingDescriptorSetLayout != nullptr);

    // Create RayTracing PipelineLayout
    FPipelineLayoutParams toneMappingPipelineLayoutParams;
    toneMappingPipelineLayoutParams.ppLayouts  = &m_pTonemappingDescriptorSetLayout;
    toneMappingPipelineLayoutParams.numLayouts = 1;
    
    m_pTonemappingPipelineLayout = FPipelineLayout::Create(m_pDevice, toneMappingPipelineLayoutParams);
    assert(m_pTonemappingPipelineLayout != nullptr);
    
    // PipelineState, RenderPass and Shaders
    FShaderModule* pVertex = FShaderModule::CreateFromFile(m_pDevice, "main", RESOURCE_PATH"/shaders/fullscreenVS.spv");
    assert(pVertex != nullptr);
    
    FShaderModule* pFragment = FShaderModule::CreateFromFile(m_pDevice, "main", RESOURCE_PATH"/shaders/tonemap.spv");
    assert(pFragment != nullptr);
    
    FRenderPassAttachment attachments[1];
    attachments[0].Format        = VK_FORMAT_R8G8B8A8_UNORM;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    attachments[0].finalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    FRenderPassParams renderPassParams = {};
    renderPassParams.ColorAttachmentCount = 1;
    renderPassParams.pColorAttachments    = attachments;
    
    m_pTonemappingRenderPass = FRenderPass::Create(m_pDevice, renderPassParams);
    assert(m_pTonemappingRenderPass != nullptr);
    
    FGraphicsPipelineStateParams tonemappingPipelineParams = {};
    tonemappingPipelineParams.pBindingDescriptions      = nullptr;
    tonemappingPipelineParams.bindingDescriptionCount   = 0;
    tonemappingPipelineParams.pAttributeDescriptions    = nullptr;
    tonemappingPipelineParams.attributeDescriptionCount = 0;
    tonemappingPipelineParams.pVertexShader             = pVertex;
    tonemappingPipelineParams.pFragmentShader           = pFragment;
    tonemappingPipelineParams.pRenderPass               = m_pTonemappingRenderPass;
    tonemappingPipelineParams.pPipelineLayout           = m_pTonemappingPipelineLayout;
    
    m_pTonemappingPipeline = FGraphicsPipeline::Create(m_pDevice, tonemappingPipelineParams);
    assert(m_pTonemappingPipeline != nullptr);
    
    delete pVertex;
    delete pFragment;
}

void FRayTracer::CreateGlobalBuffers()
{
    // Camera
    FBufferParams cameraBufferParams;
    cameraBufferParams.Size             = sizeof(FCameraBuffer);
    cameraBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    cameraBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pCameraBuffer = FBuffer::Create(m_pDevice, cameraBufferParams, m_pDeviceAllocator);
    assert(m_pCameraBuffer != nullptr);

    // Random
    FBufferParams randomBufferParams;
    randomBufferParams.Size             = sizeof(FRandomBuffer);
    randomBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    randomBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pRandomBuffer = FBuffer::Create(m_pDevice, randomBufferParams, m_pDeviceAllocator);
    assert(m_pRandomBuffer != nullptr);

    // SceneBuffer
    FBufferParams sceneBufferParams;
    sceneBufferParams.Size             = sizeof(FSceneBuffer);
    sceneBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    sceneBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pSceneBuffer = FBuffer::Create(m_pDevice, sceneBufferParams, m_pDeviceAllocator);
    assert(m_pSceneBuffer != nullptr);
    
    // SceneBuffer
    FBufferParams tonemappingBufferParams;
    tonemappingBufferParams.Size             = sizeof(FTonemappingBuffer);
    tonemappingBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    tonemappingBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pTonemappingBuffer = FBuffer::Create(m_pDevice, tonemappingBufferParams, m_pDeviceAllocator);
    assert(m_pTonemappingBuffer != nullptr);
  
    // QuadBuffer
    FBufferParams quadBufferParams;
    quadBufferParams.Size             = sizeof(FQuad) * MAX_QUAD;
    quadBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    quadBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pQuadBuffer = FBuffer::Create(m_pDevice, quadBufferParams, m_pDeviceAllocator);
    assert(m_pQuadBuffer != nullptr);
    
    // SphereBuffer
    FBufferParams sphereBufferParams;
    sphereBufferParams.Size             = sizeof(FSphere) * MAX_SPHERES;
    sphereBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    sphereBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pSphereBuffer = FBuffer::Create(m_pDevice, sphereBufferParams, m_pDeviceAllocator);
    assert(m_pSphereBuffer != nullptr);

    // PlaneBuffer
    FBufferParams planeBufferParams;
    planeBufferParams.Size             = sizeof(FPlane) * MAX_PLANES;
    planeBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    planeBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pPlaneBuffer = FBuffer::Create(m_pDevice, planeBufferParams, m_pDeviceAllocator);
    assert(m_pPlaneBuffer != nullptr);

    // VertexBuffer
    FBufferParams vertexBufferParams;
    vertexBufferParams.Size             = sizeof(FVertexRT) * MAX_TRIANGLES * 3;
    vertexBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    vertexBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pVertexBuffer = FBuffer::Create(m_pDevice, vertexBufferParams, m_pDeviceAllocator);
    assert(m_pVertexBuffer != nullptr);
    
    // TriangleBuffer
    FBufferParams triangleBufferParams;
    triangleBufferParams.Size             = sizeof(FTriangle) * MAX_TRIANGLES;
    triangleBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    triangleBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pTriangleBuffer = FBuffer::Create(m_pDevice, triangleBufferParams, m_pDeviceAllocator);
    assert(m_pTriangleBuffer != nullptr);
    
    // TriangleMeshes Buffer
    FBufferParams triangleMeshBufferParams;
    triangleMeshBufferParams.Size             = sizeof(FTriangleMesh);
    triangleMeshBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    triangleMeshBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pTriangleMeshesBuffer = FBuffer::Create(m_pDevice, triangleMeshBufferParams, m_pDeviceAllocator);
    assert(m_pTriangleMeshesBuffer != nullptr);

    // MaterialBuffer
    FBufferParams materialBufferParams;
    materialBufferParams.Size             = sizeof(FMaterial) * MAX_MATERIALS;
    materialBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    materialBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pMaterialBuffer = FBuffer::Create(m_pDevice, materialBufferParams, m_pDeviceAllocator);
    assert(m_pMaterialBuffer != nullptr);
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
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pPlaneBuffer->GetBuffer(), 8);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pMaterialBuffer->GetBuffer(), 9);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pVertexBuffer->GetBuffer(), 10);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pTriangleBuffer->GetBuffer(), 11);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pTriangleMeshesBuffer->GetBuffer(), 12);
    
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
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pPlaneBuffer->GetBuffer(), 8);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pMaterialBuffer->GetBuffer(), 9);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pVertexBuffer->GetBuffer(), 10);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pTriangleBuffer->GetBuffer(), 11);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pTriangleMeshesBuffer->GetBuffer(), 12);
    
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

void FRayTracer::CreateOrResizeSceneTexture(uint32_t width, uint32_t height)
{
    if (m_pSceneTexture0)
    {
        if ((m_pSceneTexture0->GetWidth() == width && m_pSceneTexture0->GetHeight() == height) || width == 0 || height == 0)
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
    FTextureParams textureParams = {};
    textureParams.Format        = VK_FORMAT_R32G32B32A32_SFLOAT;
    textureParams.ImageType     = VK_IMAGE_TYPE_2D;
    textureParams.Width         = m_ViewportWidth  = width;
    textureParams.Height        = m_ViewportHeight = height;
    textureParams.Usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    textureParams.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Scene texture frame 0
    m_pSceneTexture0 = FTexture::Create(m_pDevice, textureParams);
    assert(m_pSceneTexture0 != nullptr);
    SetDebugName(m_pDevice->GetDevice(), "SceneTexture0", reinterpret_cast<uint64_t>(m_pSceneTexture0->GetImage()), VK_OBJECT_TYPE_IMAGE);

    {
        FTextureViewParams textureViewParams = {};
        textureViewParams.pTexture = m_pSceneTexture0;

        m_pSceneTextureView0 = FTextureView::Create(m_pDevice, textureViewParams);
        assert(m_pSceneTextureView0 != nullptr);
        SetDebugName(m_pDevice->GetDevice(), "SceneTextureView0", reinterpret_cast<uint64_t>(m_pSceneTextureView0->GetImageView()), VK_OBJECT_TYPE_IMAGE_VIEW);
    }

    // Scene texture frame 1
    m_pSceneTexture1 = FTexture::Create(m_pDevice, textureParams);
    assert(m_pSceneTexture1 != nullptr);
    SetDebugName(m_pDevice->GetDevice(), "SceneTexture1", reinterpret_cast<uint64_t>(m_pSceneTexture1->GetImage()), VK_OBJECT_TYPE_IMAGE);
    
    {
        FTextureViewParams textureViewParams = {};
        textureViewParams.pTexture = m_pSceneTexture1;
        
        m_pSceneTextureView1 = FTextureView::Create(m_pDevice, textureViewParams);
        assert(m_pSceneTextureView1 != nullptr);
        SetDebugName(m_pDevice->GetDevice(), "SceneTextureView1", reinterpret_cast<uint64_t>(m_pSceneTextureView1->GetImageView()), VK_OBJECT_TYPE_IMAGE_VIEW);
    }

    // Create texture for the viewport
    FTextureParams outputTextureParams = {};
    outputTextureParams.Format        = VK_FORMAT_R8G8B8A8_UNORM;
    outputTextureParams.ImageType     = VK_IMAGE_TYPE_2D;
    outputTextureParams.Width         = m_ViewportWidth  = width;
    outputTextureParams.Height        = m_ViewportHeight = height;
    outputTextureParams.Usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    outputTextureParams.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    m_pOutputTexture = FTexture::Create(m_pDevice, outputTextureParams);
    assert(m_pOutputTexture != nullptr);
    SetDebugName(m_pDevice->GetDevice(), "OutputTexture", reinterpret_cast<uint64_t>(m_pOutputTexture->GetImage()), VK_OBJECT_TYPE_IMAGE);
    
    {
        FTextureViewParams textureViewParams = {};
        textureViewParams.pTexture = m_pOutputTexture;
        
        m_pOutputTextureView = FTextureView::Create(m_pDevice, textureViewParams);
        assert(m_pOutputTextureView != nullptr);
        SetDebugName(m_pDevice->GetDevice(), "OutputTextureView", reinterpret_cast<uint64_t>(m_pOutputTextureView->GetImageView()), VK_OBJECT_TYPE_IMAGE_VIEW);
    }
    
    // Create Framebuffer for the tonemap stage
    VkImageView imageView = m_pOutputTextureView->GetImageView();
    FFramebufferParams framebufferParams = {};
    framebufferParams.AttachMentCount = 1;
    framebufferParams.Width           = m_ViewportWidth;
    framebufferParams.Height          = m_ViewportHeight;
    framebufferParams.pRenderPass     = m_pTonemappingRenderPass;
    framebufferParams.pAttachMents    = &imageView;

    m_pTonemappingFramebuffer = FFramebuffer::Create(m_pDevice, framebufferParams);
    
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
