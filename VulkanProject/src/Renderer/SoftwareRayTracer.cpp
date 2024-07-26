#include "SoftwareRayTracer.h"
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
#include "Vulkan/BindlessManager.h"

FSoftwareRayTracer::FSoftwareRayTracer()
    : FBaseRenderer()
    , m_pRayTracingPipeline()
    , m_pRayTracingPipelineLayout(nullptr)
    , m_pRayTracingDescriptorSetLayout(nullptr)
    , m_pTonemappingPipeline(nullptr)
    , m_pTonemappingRenderPass(nullptr)
    , m_pTonemappingPipelineLayout(nullptr)
    , m_pTonemappingDescriptorSetLayout(nullptr)
    , m_pTonemappingDescriptorSet0(nullptr)
    , m_pTonemappingDescriptorSet1(nullptr)
    , m_pTonemappingFramebuffer(nullptr)
    , m_pDebugPipeline(nullptr)
    , m_pDebugPipelineWireframe(nullptr)
    , m_pDebugAABBPipeline(nullptr)
    , m_pDebugRenderPass(nullptr)
    , m_pDebugPipelineLayout(nullptr)
    , m_pDebugAABBPipelineLayout(nullptr)
    , m_pDebugDescriptorSetLayout(nullptr)
    , m_pDebugDescriptorSet0(nullptr)
    , m_pDebugDescriptorSet1(nullptr)
    , m_pDebugFramebuffer(nullptr)
    , m_DebugDepth(0)
    , m_pRayTracingDescriptorSet0(nullptr)
    , m_pCameraBuffer(nullptr)
    , m_pRandomBuffer(nullptr)
    , m_pSceneBuffer(nullptr)
    , m_pTonemappingBuffer(nullptr)
    , m_pSphereBuffer(nullptr)
    , m_pQuadBuffer(nullptr)
    , m_pTriangleBuffer(nullptr)
    , m_pMeshBuffer(nullptr)
    , m_pVertexBuffer(nullptr)
    , m_pVertexExBuffer(nullptr)
    , m_pMaterialBuffer(nullptr)
    , m_pBvhBuffer(nullptr)
    , m_pAABBVertexBuffer(nullptr)
    , m_pAABBIndexBuffer(nullptr)
    , m_pAABBInstanceBuffer(nullptr)
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

FSoftwareRayTracer::~FSoftwareRayTracer()
{
}

void FSoftwareRayTracer::Init(FDevice* pDevice, FSwapchain* pSwapchain)
{
    FBaseRenderer::Init(pDevice, pSwapchain);

    // Create Skybox
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
        m_pSkyboxSampler->SetDebugName("Skybox Sampler");
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
        m_pTonemapSampler->SetDebugName("Tonemapping Sampler");
    }

    // Bindless Manager
    m_SkyboxBindlessIndex = m_pDevice->GetBindlessManager().AddImageView(m_pSkybox->GetTextureView()->GetImageView(), m_pSkyboxSampler->GetSampler());

    // Create scene
    m_pScene = new FSphereScene(ESphereSceneType::Default);
    m_pScene->Initialize();

    // RenderPasses
    CreateRayTracingResources();
    CreateTonemappingResources();
    CreateDebugViewResources();
    
    // Create all buffers
    CreateGlobalBuffers();

    // Create the scene texture
    m_ViewportWidth  = 0;
    m_ViewportHeight = 0;
    CreateOrResizeSceneTexture(1280, 720);
}

void FSoftwareRayTracer::Tick(float DeltaTime)
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
    m_pScene->m_Camera.Update(m_pScene->m_Settings.FieldOfView, m_pSceneTexture0->GetWidth(), m_pSceneTexture0->GetHeight(), 0.01f, 10000.0f);

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

    // Update CameraBuffer
    FCameraBuffer CameraBuffer = {};
    CameraBuffer.Projection         = m_pScene->m_Camera.GetProjectionMatrix();
    CameraBuffer.View               = m_pScene->m_Camera.GetViewMatrix();
    CameraBuffer.InverseView        = m_pScene->m_Camera.GetInverseViewMatrix();
    CameraBuffer.InverseProjection  = m_pScene->m_Camera.GetInverseProjectionMatrix();
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
    
    // Update global buffers
    UpdateGlobalBuffers(pCurrentCommandBuffer);
    
    if (m_pScene->m_Settings.ViewMode != EViewMode::Debug)
    {
        // Perform RayTracing
        PerformRayTracing(pCurrentCommandBuffer);
        
        // Tonemapping
        PerformTonemapping(pCurrentCommandBuffer);
    }
    else
    {
        // Rasterize triangle models and display the BVH
        PerformDebugPass(pCurrentCommandBuffer);
    }

    // End CommandBuffer
    pCurrentCommandBuffer->WriteTimestamp(pCurrentTimestampQuery, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1);
    pCurrentCommandBuffer->End();

    m_pDevice->ExecuteGraphics(pCurrentCommandBuffer, nullptr, nullptr);
}

void FSoftwareRayTracer::PerformRayTracing(FCommandBuffer* pCommandBuffer)
{
    pCommandBuffer->TransitionImage(m_pSceneTexture0->GetImage(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
    pCommandBuffer->TransitionImage(m_pSceneTexture1->GetImage(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);

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

        pCommandBuffer->ClearColorImage(m_pSceneTexture0->GetImage(), VK_IMAGE_LAYOUT_GENERAL, &ClearColor, 1, &SubresourceRange);
        pCommandBuffer->ClearColorImage(m_pSceneTexture1->GetImage(), VK_IMAGE_LAYOUT_GENERAL, &ClearColor, 1, &SubresourceRange);

        m_bResetImage = false;
        m_FrameIndex  = 0;
    }
    else
    {
        m_FrameIndex++;
    }

    // Update Scene
    FSceneBuffer SceneBuffer = {};
    SceneBuffer.NumQuads              = m_pScene->m_Quads.size();
    SceneBuffer.NumSpheres            = m_pScene->m_Spheres.size();
    SceneBuffer.NumMaterials          = m_pScene->m_GpuMaterials.size();
    SceneBuffer.NumMeshes             = m_pScene->m_Meshes.size();
    SceneBuffer.NumBvhNodes           = m_pScene->m_AccelerationStructure.m_BoundingBoxes.size();
    SceneBuffer.NumTriangles          = m_pScene->m_AccelerationStructure.m_Triangles.size();
    SceneBuffer.BackgroundType        = m_pScene->m_Settings.BackgroundType;
    SceneBuffer.NumBounces            = m_pScene->m_Settings.NumBounces;
    SceneBuffer.ViewMode              = static_cast<uint32_t>(m_pScene->m_Settings.ViewMode);
    SceneBuffer.GradientLightStrength = m_pScene->m_Settings.GradientLightStrength;
    
    pCommandBuffer->UpdateBuffer(m_pSceneBuffer, 0, sizeof(FSceneBuffer), &SceneBuffer);

    // Bind pipeline and descriptorSet
    pCommandBuffer->BindComputePipelineState(m_pRayTracingPipeline.load());
    
    const uint64_t Frame = (m_FrameIndex % 2);
    if (Frame == 0)
    {
        pCommandBuffer->BindComputeDescriptorSet(m_pRayTracingPipelineLayout, m_pRayTracingDescriptorSet0, 0);
    }
    else
    {
        pCommandBuffer->BindComputeDescriptorSet(m_pRayTracingPipelineLayout, m_pRayTracingDescriptorSet1, 0);
    }
    
    pCommandBuffer->BindBindlessDescriptors(m_pRayTracingPipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE);

    // Dispatch RayTracing
    const uint32_t Threads = 16;
    VkExtent2D DispatchSize = { Math::AlignUp(m_pSceneTexture0->GetWidth(), Threads) / Threads, Math::AlignUp(m_pSceneTexture0->GetHeight(), Threads) / Threads };
    pCommandBuffer->Dispatch(DispatchSize.width, DispatchSize.height, 1);

    pCommandBuffer->TransitionImage(m_pSceneTexture0->GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    pCommandBuffer->TransitionImage(m_pSceneTexture1->GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
}

void FSoftwareRayTracer::PerformTonemapping(FCommandBuffer* pCommandBuffer)
{
    // Update Tonemapping Settings
    FTonemappingBuffer TonemappingBuffer = {};
    TonemappingBuffer.Exposure = m_pScene->m_Settings.Exposure;
    pCommandBuffer->UpdateBuffer(m_pTonemappingBuffer, 0, sizeof(FTonemappingBuffer), &TonemappingBuffer);
    
    // Begin renderpass
    VkClearValue ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    pCommandBuffer->BeginRenderPass(m_pTonemappingRenderPass, m_pTonemappingFramebuffer, &ClearColor, 1);
    
    // Set viewport
    VkViewport Viewport = { 0.0f, 0.0f, float(m_ViewportWidth), float(m_ViewportHeight), 0.0f, 1.0f };
    pCommandBuffer->SetViewport(Viewport);
    
    VkRect2D scissor = { { 0, 0}, { m_ViewportWidth, m_ViewportHeight } };
    pCommandBuffer->SetScissorRect(scissor);
    
    // Bind pipeline
    pCommandBuffer->BindGraphicsPipelineState(m_pTonemappingPipeline);

    // Perform tonemapping
    const uint64_t Frame = (m_FrameIndex % 2);
    if (Frame == 0)
    {
        pCommandBuffer->BindGraphicsDescriptorSet(m_pTonemappingPipelineLayout, m_pTonemappingDescriptorSet0, 0);
    }
    else
    {
        pCommandBuffer->BindGraphicsDescriptorSet(m_pTonemappingPipelineLayout, m_pTonemappingDescriptorSet1, 0);
    }

    // Draw
    pCommandBuffer->DrawInstanced(3, 1, 0, 0);
    
    // End renderpass
    pCommandBuffer->EndRenderPass();
}

void FSoftwareRayTracer::PerformDebugPass(FCommandBuffer* pCommandBuffer)
{
    // Define clear colors
    VkClearValue ClearColor[2];
    ClearColor[0].color        = { 0.0f, 0.0f, 0.0f, 1.0f };
    ClearColor[1].depthStencil = { 1.0f, 0 };

    if (!m_pScene->m_pMeshVertexBuffer || !m_pScene->m_pMeshIndexBuffer)
    {
        // Begin renderpass (Only clear the image when the buffers are invalid)
        pCommandBuffer->BeginRenderPass(m_pDebugRenderPass, m_pDebugFramebuffer, ClearColor, 2);
        
        // End renderpass
        pCommandBuffer->EndRenderPass();
        return;
    }
    
    // Begin renderpass
    pCommandBuffer->BeginRenderPass(m_pDebugRenderPass, m_pDebugFramebuffer, ClearColor, 2);
    
    // Set viewport
    VkViewport Viewport;
    Viewport.width    =  static_cast<float>(m_ViewportWidth);
    Viewport.height   =  -static_cast<float>(m_ViewportHeight);
    Viewport.minDepth =  0.0f;
    Viewport.maxDepth =  1.0f;
    Viewport.x        =  0.0f;
    Viewport.y        =  static_cast<float>(m_ViewportHeight);
    
    pCommandBuffer->SetViewport(Viewport);
    
    VkRect2D scissor = { { 0, 0}, { m_ViewportWidth, m_ViewportHeight } };
    pCommandBuffer->SetScissorRect(scissor);
    
    // Draw Mesh
    {
        pCommandBuffer->BindGraphicsPipelineState(m_pDebugPipeline);
        
        const glm::vec4 Color = glm::vec4(0.9f, 0.9f, 0.9f, 1.0f);
        pCommandBuffer->PushConstants(m_pDebugPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(glm::vec4), glm::value_ptr(Color));
        
        // Bind DescriptorSets
        const uint64_t Frame = (m_FrameIndex % 2);
        if (Frame == 0)
        {
            pCommandBuffer->BindGraphicsDescriptorSet(m_pDebugPipelineLayout, m_pDebugDescriptorSet0, 0);
        }
        else
        {
            pCommandBuffer->BindGraphicsDescriptorSet(m_pDebugPipelineLayout, m_pDebugDescriptorSet1, 0);
        }
        
        // Set Vertex- and IndexBuffer
        pCommandBuffer->BindVertexBuffer(m_pScene->m_pMeshVertexBuffer, 0, 0);
        pCommandBuffer->BindIndexBuffer(m_pScene->m_pMeshIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

        // Draw
        const size_t IndexCount = m_pScene->m_Indicies.size();
        pCommandBuffer->DrawIndexInstanced(IndexCount, 1, 0, 0, 0);
    }
    
    // Draw wire-frame
    {
        pCommandBuffer->BindGraphicsPipelineState(m_pDebugPipelineWireframe);

        const glm::vec4 Color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        pCommandBuffer->PushConstants(m_pDebugPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(glm::vec4), glm::value_ptr(Color));
        
        // Bind DescriptorSets
        const uint64_t Frame = (m_FrameIndex % 2);
        if (Frame == 0)
        {
            pCommandBuffer->BindGraphicsDescriptorSet(m_pDebugPipelineLayout, m_pDebugDescriptorSet0, 0);
        }
        else
        {
            pCommandBuffer->BindGraphicsDescriptorSet(m_pDebugPipelineLayout, m_pDebugDescriptorSet1, 0);
        }
        
        // Set Vertex- and IndexBuffer
        pCommandBuffer->BindVertexBuffer(m_pScene->m_pMeshVertexBuffer, 0, 0);
        pCommandBuffer->BindIndexBuffer(m_pScene->m_pMeshIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

        // Draw
        const size_t IndexCount = m_pScene->m_Indicies.size();
        pCommandBuffer->DrawIndexInstanced(IndexCount, 1, 0, 0, 0);
    }
    
    // Draw the bounding boxes
    {
        pCommandBuffer->BindGraphicsPipelineState(m_pDebugAABBPipeline);

        // Bind DescriptorSets
        const uint64_t Frame = (m_FrameIndex % 2);
        if (Frame == 0)
        {
            pCommandBuffer->BindGraphicsDescriptorSet(m_pDebugAABBPipelineLayout, m_pDebugDescriptorSet0, 0);
        }
        else
        {
            pCommandBuffer->BindGraphicsDescriptorSet(m_pDebugAABBPipelineLayout, m_pDebugDescriptorSet1, 0);
        }

        pCommandBuffer->BindVertexBuffer(m_pAABBVertexBuffer, 0, 0);
        pCommandBuffer->BindIndexBuffer(m_pAABBIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

        struct FAABBDebugData
        {
            glm::vec4 Color;
        } DebugData;

        DebugData.Color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        pCommandBuffer->PushConstants(m_pDebugAABBPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(FAABBDebugData), &DebugData);

        const uint32_t NumInstances = static_cast<uint32_t>(m_pScene->m_AccelerationStructure.m_BoundingBoxes.size());
        pCommandBuffer->DrawIndexInstanced(m_AABBIndexCount, NumInstances, 0, 0, 0);
    }
    
    // End RenderPass
    pCommandBuffer->EndRenderPass();
}

void FSoftwareRayTracer::OnRenderUI()
{
    // Setup DockSpace
    static ImGuiDockNodeFlags DockspaceFlags = ImGuiDockNodeFlags_None;

    const ImGuiViewport* pMainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(pMainViewport->WorkPos);
    ImGui::SetNextWindowSize(pMainViewport->WorkSize);
    ImGui::SetNextWindowViewport(pMainViewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoDocking;
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
                "Albedo",
                "Barycentrics",
                "TexCoords",
                "BVH Intersection",
                "Debug"
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
                    m_pScene->m_Settings.ViewMode = EViewMode::Albedo;
                }
                else if (CurrentViewMode == 3)
                {
                    m_pScene->m_Settings.ViewMode = EViewMode::Barycentrics;
                }
                else if (CurrentViewMode == 4)
                {
                    m_pScene->m_Settings.ViewMode = EViewMode::TexCoords;
                }
                else if (CurrentViewMode == 5)
                {
                    m_pScene->m_Settings.ViewMode = EViewMode::BVHIntersection;
                }
                else if (CurrentViewMode == 6)
                {
                    m_pScene->m_Settings.ViewMode = EViewMode::Debug;
                }

                PrevViewMode = CurrentViewMode;
                m_bResetImage = true;
            }

            if (CurrentViewMode == 3)
            {
                ImGui::DragInt("Debug Depth", &m_DebugDepth, 1, 0, m_pScene->m_AccelerationStructure.Stats.Depth, "%d", ImGuiSliderFlags_AlwaysClamp);
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

            if (CurrentBG == 1)
            {
                float Strength = m_pScene->m_Settings.GradientLightStrength;
                if (ImGui::DragFloat("Gradient Strength", &Strength, 0.1f, 1.0f, 100.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
                {
                    m_pScene->m_Settings.GradientLightStrength = Strength;
                    m_bResetImage = true;
                }
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
            for (FShaderMaterial& Material : m_pScene->m_GpuMaterials)
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

void FSoftwareRayTracer::RenderSceneUI()
{
}

void FSoftwareRayTracer::Release()
{
    // Wait for device before we destroy anything
    m_pDevice->WaitForIdle();

    ReleaseDescriptorSets();

    SAFE_DELETE(m_pCameraBuffer);
    SAFE_DELETE(m_pRandomBuffer);
    SAFE_DELETE(m_pSceneBuffer);
    SAFE_DELETE(m_pTonemappingBuffer);
    SAFE_DELETE(m_pQuadBuffer);
    SAFE_DELETE(m_pSphereBuffer);
    SAFE_DELETE(m_pVertexBuffer);
    SAFE_DELETE(m_pVertexExBuffer);
    SAFE_DELETE(m_pTriangleBuffer);
    SAFE_DELETE(m_pMeshBuffer);
    SAFE_DELETE(m_pMaterialBuffer);
    SAFE_DELETE(m_pBvhBuffer);
    SAFE_DELETE(m_pAABBVertexBuffer);
    SAFE_DELETE(m_pAABBIndexBuffer);
    SAFE_DELETE(m_pAABBInstanceBuffer);
    
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

    SAFE_DELETE(m_pDebugPipeline);
    SAFE_DELETE(m_pDebugPipelineWireframe);
    SAFE_DELETE(m_pDebugAABBPipeline);
    SAFE_DELETE(m_pDebugRenderPass);
    SAFE_DELETE(m_pDebugPipelineLayout);
    SAFE_DELETE(m_pDebugDescriptorSetLayout);
    SAFE_DELETE(m_pDebugAABBPipelineLayout);

    SAFE_DELETE(m_pSceneTexture1);
    SAFE_DELETE(m_pSceneTextureView1);
    SAFE_DELETE(m_pSceneTexture0);
    SAFE_DELETE(m_pSceneTextureView0);
    SAFE_DELETE(m_pOutputTexture);
    SAFE_DELETE(m_pOutputTextureView);
    SAFE_DELETE(m_pDepthBufferTexture);
    SAFE_DELETE(m_pDepthBufferTextureView);
    SAFE_DELETE(m_pTonemappingFramebuffer);
    SAFE_DELETE(m_pDebugFramebuffer);
    
    SAFE_DELETE(m_pScene);

    FBaseRenderer::Release();
}

void FSoftwareRayTracer::OnWindowResize(uint32_t Width, uint32_t Height)
{
}

void FSoftwareRayTracer::CreateRayTracingResources()
{
    // Create RayTracing DescriptorSetLayout
    constexpr uint32_t NumRayTracingBindings = 14;
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
    
    // VertexEx Buffer
    RayTracingBindings[10].binding            = 10;
    RayTracingBindings[10].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[10].descriptorCount    = 1;
    RayTracingBindings[10].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[10].pImmutableSamplers = nullptr;
    
    // Triangles Buffer
    RayTracingBindings[11].binding            = 11;
    RayTracingBindings[11].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[11].descriptorCount    = 1;
    RayTracingBindings[11].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[11].pImmutableSamplers = nullptr;
    
    // TriangleMeshes Buffer
    RayTracingBindings[12].binding            = 12;
    RayTracingBindings[12].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[12].descriptorCount    = 1;
    RayTracingBindings[12].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[12].pImmutableSamplers = nullptr;

    // Bvh Buffer
    RayTracingBindings[13].binding            = 13;
    RayTracingBindings[13].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[13].descriptorCount    = 1;
    RayTracingBindings[13].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[13].pImmutableSamplers = nullptr;

    FDescriptorSetLayoutParams RayTracingDescriptorSetLayoutParams;
    RayTracingDescriptorSetLayoutParams.pBindings   = RayTracingBindings;
    RayTracingDescriptorSetLayoutParams.NumBindings = NumRayTracingBindings;

    m_pRayTracingDescriptorSetLayout = FDescriptorSetLayout::Create(m_pDevice, RayTracingDescriptorSetLayoutParams);
    assert(m_pRayTracingDescriptorSetLayout != nullptr);
    m_pRayTracingDescriptorSetLayout->SetDebugName("RayTracingPass DescriptorSetLayout");

    // Create RayTracing PipelineLayout
    FPipelineLayoutParams RayTracingPipelineLayoutParams;
    RayTracingPipelineLayoutParams.ppLayouts       = &m_pRayTracingDescriptorSetLayout;
    RayTracingPipelineLayoutParams.NumLayouts      = 1;
    RayTracingPipelineLayoutParams.bEnableBindless = true;
    
    m_pRayTracingPipelineLayout = FPipelineLayout::Create(m_pDevice, RayTracingPipelineLayoutParams);
    assert(m_pRayTracingPipelineLayout != nullptr);
    m_pRayTracingPipelineLayout->SetDebugName("RayTracingPass PipelineLayout");

    // Create RayTracing shader and pipeline
    FShaderModule* pComputeShader = FShaderModule::CreateFromFile(m_pDevice, "main", RESOURCE_PATH"/shaders/raytracer.spv");
    assert(pComputeShader != nullptr);
    pComputeShader->SetDebugName(RESOURCE_PATH"/shaders/raytracer.spv");
    
    FComputePipelineStateParams PipelineParams = {};
    PipelineParams.pShader         = pComputeShader;
    PipelineParams.pPipelineLayout = m_pRayTracingPipelineLayout;
    
    m_pRayTracingPipeline = FComputePipeline::Create(m_pDevice, PipelineParams);
    assert(m_pRayTracingPipeline != nullptr);
    m_pRayTracingPipeline.load()->SetDebugName("RayTracingPass Pipeline");
    
    delete pComputeShader;
}

void FSoftwareRayTracer::CreateDebugViewResources()
{
    // Create DebugView DescriptorSetLayout
    constexpr uint32_t NumDebugPassBindings = 2;
    VkDescriptorSetLayoutBinding DebugPassBindings[NumDebugPassBindings];

    // Camera Buffer
    DebugPassBindings[0].binding            = 0;
    DebugPassBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    DebugPassBindings[0].descriptorCount    = 1;
    DebugPassBindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    DebugPassBindings[0].pImmutableSamplers = nullptr;
    
    // Storage Buffer
    DebugPassBindings[1].binding            = 1;
    DebugPassBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    DebugPassBindings[1].descriptorCount    = 1;
    DebugPassBindings[1].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    DebugPassBindings[1].pImmutableSamplers = nullptr;
    
    FDescriptorSetLayoutParams DebugPassDescriptorSetLayoutParams;
    DebugPassDescriptorSetLayoutParams.pBindings   = DebugPassBindings;
    DebugPassDescriptorSetLayoutParams.NumBindings = NumDebugPassBindings;

    m_pDebugDescriptorSetLayout = FDescriptorSetLayout::Create(m_pDevice, DebugPassDescriptorSetLayoutParams);
    assert(m_pDebugDescriptorSetLayout != nullptr);
    m_pDebugDescriptorSetLayout->SetDebugName("DebugPass DescriptorSetLayout");

    // Create DebugPass PipelineLayout
    FPipelineLayoutParams DebugPassPipelineLayoutParams;
    DebugPassPipelineLayoutParams.ppLayouts        = &m_pDebugDescriptorSetLayout;
    DebugPassPipelineLayoutParams.NumLayouts       = 1;
    DebugPassPipelineLayoutParams.NumPushConstants = 4;
    
    m_pDebugPipelineLayout = FPipelineLayout::Create(m_pDevice, DebugPassPipelineLayoutParams);
    assert(m_pDebugPipelineLayout != nullptr);
    m_pDebugPipelineLayout->SetDebugName("DebugPass PipelineLayout");
    
    DebugPassPipelineLayoutParams.NumPushConstants = 20;
    
    m_pDebugAABBPipelineLayout = FPipelineLayout::Create(m_pDevice, DebugPassPipelineLayoutParams);
    assert(m_pDebugAABBPipelineLayout != nullptr);
    m_pDebugAABBPipelineLayout->SetDebugName("DebugPass AABB PipelineLayout");

    // PipelineState, RenderPass and Shaders
    FShaderModule* pVertex = FShaderModule::CreateFromFile(m_pDevice, "main", RESOURCE_PATH"/shaders/vertex.spv");
    assert(pVertex != nullptr);
    pVertex->SetDebugName(RESOURCE_PATH"/shaders/vertex.spv");

    FShaderModule* pAABBVertex = FShaderModule::CreateFromFile(m_pDevice, "main", RESOURCE_PATH"/shaders/aabb_debug_vs.spv");
    assert(pAABBVertex != nullptr);
    pAABBVertex->SetDebugName(RESOURCE_PATH"/shaders/aabb_debug_vs.spv");

    FShaderModule* pFragment = FShaderModule::CreateFromFile(m_pDevice, "main", RESOURCE_PATH"/shaders/fragment.spv");
    assert(pFragment != nullptr);
    pFragment->SetDebugName(RESOURCE_PATH"/shaders/fragment.spv");

    FShaderModule* pAABBFragment = FShaderModule::CreateFromFile(m_pDevice, "main", RESOURCE_PATH"/shaders/aabb_debug_fs.spv");
    assert(pAABBFragment != nullptr);
    pAABBFragment->SetDebugName(RESOURCE_PATH"/shaders/aabb_debug_fs.spv");

    FRenderPassAttachment ColorAttachments[1];
    ColorAttachments[0].Format        = VK_FORMAT_R8G8B8A8_UNORM;
    ColorAttachments[0].InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ColorAttachments[0].FinalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    FRenderPassAttachment DepthAttachment[1];
    DepthAttachment[0].Format        = VK_FORMAT_D24_UNORM_S8_UINT;
    DepthAttachment[0].InitialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    DepthAttachment[0].FinalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    FRenderPassParams RenderPassParams = {};
    RenderPassParams.pColorAttachments    = ColorAttachments;
    RenderPassParams.ColorAttachmentCount = 1;
    RenderPassParams.pDepthAttachment     = DepthAttachment;
    
    m_pDebugRenderPass = FRenderPass::Create(m_pDevice, RenderPassParams);
    assert(m_pDebugRenderPass != nullptr);
    m_pDebugRenderPass->SetDebugName("DebugPass RenderPass");
    
    FGraphicsPipelineStateParams DebugPassPipelineParams = {};
    DebugPassPipelineParams.pBindingDescriptions      = FVertexPosOnly::GetBindingDescription();
    DebugPassPipelineParams.BindingDescriptionCount   = 1;
    DebugPassPipelineParams.pAttributeDescriptions    = FVertexPosOnly::GetAttributeDescriptions();
    DebugPassPipelineParams.AttributeDescriptionCount = 1;
    DebugPassPipelineParams.pVertexShader             = pVertex;
    DebugPassPipelineParams.pFragmentShader           = pFragment;
    DebugPassPipelineParams.pRenderPass               = m_pDebugRenderPass;
    DebugPassPipelineParams.pPipelineLayout           = m_pDebugPipelineLayout;
    DebugPassPipelineParams.bDepthEnable              = true;
    
    m_pDebugPipeline = FGraphicsPipeline::Create(m_pDevice, DebugPassPipelineParams);
    assert(m_pDebugPipeline != nullptr);
    m_pDebugPipeline->SetDebugName("DebugPass Pipeline");

    DebugPassPipelineParams.PolygonMode = VK_POLYGON_MODE_LINE;
    
    m_pDebugPipelineWireframe = FGraphicsPipeline::Create(m_pDevice, DebugPassPipelineParams);
    assert(m_pDebugPipelineWireframe != nullptr);
    m_pDebugPipelineWireframe->SetDebugName("DebugPass Pipeline Wireframe");

    DebugPassPipelineParams.pBindingDescriptions      = FVertexAABB::GetBindingDescription();
    DebugPassPipelineParams.BindingDescriptionCount   = 1;
    DebugPassPipelineParams.pAttributeDescriptions    = FVertexAABB::GetAttributeDescriptions();
    DebugPassPipelineParams.AttributeDescriptionCount = 1;
    DebugPassPipelineParams.pVertexShader             = pAABBVertex;
    DebugPassPipelineParams.pFragmentShader           = pAABBFragment;
    DebugPassPipelineParams.pRenderPass               = m_pDebugRenderPass;
    DebugPassPipelineParams.pPipelineLayout           = m_pDebugAABBPipelineLayout;
    DebugPassPipelineParams.Topology                  = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    DebugPassPipelineParams.bDepthEnable              = true;
    
    m_pDebugAABBPipeline = FGraphicsPipeline::Create(m_pDevice, DebugPassPipelineParams);
    assert(m_pDebugAABBPipeline != nullptr);
    m_pDebugAABBPipeline->SetDebugName("DebugPass AABB Pipeline");

    delete pVertex;
    delete pAABBVertex;
    delete pFragment;
    delete pAABBFragment;

    std::array<glm::vec3, 8> AABBVertices =
    {
        glm::vec3(-0.5f, -0.5f,  0.5f),
        glm::vec3( 0.5f, -0.5f,  0.5f),
        glm::vec3(-0.5f,  0.5f,  0.5f),
        glm::vec3( 0.5f,  0.5f,  0.5f),
        glm::vec3( 0.5f, -0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f, -0.5f),
        glm::vec3( 0.5f,  0.5f, -0.5f),
        glm::vec3(-0.5f,  0.5f, -0.5f)
    };

    std::array<uint32_t, 24> AABBIndices =
    {
        0, 1,
        1, 3,
        3, 2,
        2, 0,
        1, 4,
        3, 6,
        6, 4,
        4, 5,
        5, 7,
        7, 6,
        0, 5,
        2, 7,
    };
    
    // BVH Buffers
    FBufferParams BufferParams;
    BufferParams.Size             = sizeof(glm::vec3) * AABBVertices.size();
    BufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;
    BufferParams.Usage            = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    m_pAABBVertexBuffer = FBuffer::CreateWithData(FApplication::Get().GetDevice(), BufferParams, nullptr, AABBVertices.data());
    assert(m_pAABBVertexBuffer != nullptr);
    m_pAABBVertexBuffer->SetDebugName("AABBVertexBuffer");

    BufferParams.Size  = sizeof(uint32_t) * AABBIndices.size();
    BufferParams.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    m_AABBIndexCount = AABBIndices.size();
    
    m_pAABBIndexBuffer = FBuffer::CreateWithData(FApplication::Get().GetDevice(), BufferParams, nullptr, AABBIndices.data());
    assert(m_pAABBIndexBuffer != nullptr);
    m_pAABBIndexBuffer->SetDebugName("AABBIndexBuffer");
}

void FSoftwareRayTracer::CreateTonemappingResources()
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

    // Settings Buffer
    TonemappingBindings[1].binding            = 1;
    TonemappingBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    TonemappingBindings[1].descriptorCount    = 1;
    TonemappingBindings[1].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    TonemappingBindings[1].pImmutableSamplers = nullptr;
    
    FDescriptorSetLayoutParams TonemappingDescriptorSetLayoutParams;
    TonemappingDescriptorSetLayoutParams.pBindings   = TonemappingBindings;
    TonemappingDescriptorSetLayoutParams.NumBindings = NumTonemappingBindings;

    m_pTonemappingDescriptorSetLayout = FDescriptorSetLayout::Create(m_pDevice, TonemappingDescriptorSetLayoutParams);
    assert(m_pTonemappingDescriptorSetLayout != nullptr);
    m_pTonemappingDescriptorSetLayout->SetDebugName("TonemappingPass DescriptorSetLayout");

    // Create Tonemapping PipelineLayout
    FPipelineLayoutParams ToneMappingPipelineLayoutParams;
    ToneMappingPipelineLayoutParams.ppLayouts  = &m_pTonemappingDescriptorSetLayout;
    ToneMappingPipelineLayoutParams.NumLayouts = 1;
    
    m_pTonemappingPipelineLayout = FPipelineLayout::Create(m_pDevice, ToneMappingPipelineLayoutParams);
    assert(m_pTonemappingPipelineLayout != nullptr);
    m_pTonemappingPipelineLayout->SetDebugName("TonemappingPass PipelineLayout");

    // PipelineState, RenderPass and Shaders
    FShaderModule* pVertex = FShaderModule::CreateFromFile(m_pDevice, "main", RESOURCE_PATH"/shaders/fullscreenVS.spv");
    assert(pVertex != nullptr);
    pVertex->SetDebugName(RESOURCE_PATH"/shaders/fullscreenVS.spv");

    FShaderModule* pFragment = FShaderModule::CreateFromFile(m_pDevice, "main", RESOURCE_PATH"/shaders/tonemap.spv");
    assert(pFragment != nullptr);
    pFragment->SetDebugName(RESOURCE_PATH"/shaders/tonemap.spv");

    FRenderPassAttachment Attachments[1];
    Attachments[0].Format        = VK_FORMAT_R8G8B8A8_UNORM;
    Attachments[0].InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    Attachments[0].FinalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    FRenderPassParams RenderPassParams = {};
    RenderPassParams.ColorAttachmentCount = 1;
    RenderPassParams.pColorAttachments    = Attachments;
    
    m_pTonemappingRenderPass = FRenderPass::Create(m_pDevice, RenderPassParams);
    assert(m_pTonemappingRenderPass != nullptr);
    m_pTonemappingRenderPass->SetDebugName("TonemappingPass RenderPass");

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
    m_pTonemappingPipeline->SetDebugName("TonemappingPass Pipeline");

    delete pVertex;
    delete pFragment;
}

void FSoftwareRayTracer::CreateGlobalBuffers()
{
    // Camera
    FBufferParams CameraBufferParams;
    CameraBufferParams.Size             = sizeof(FCameraBuffer);
    CameraBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    CameraBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pCameraBuffer = FBuffer::Create(m_pDevice, CameraBufferParams, m_pDeviceAllocator);
    assert(m_pCameraBuffer != nullptr);
    m_pCameraBuffer->SetDebugName("Camera-Buffer");
    
    // Random
    FBufferParams RandomBufferParams;
    RandomBufferParams.Size             = sizeof(FRandomBuffer);
    RandomBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    RandomBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pRandomBuffer = FBuffer::Create(m_pDevice, RandomBufferParams, m_pDeviceAllocator);
    assert(m_pRandomBuffer != nullptr);
    m_pRandomBuffer->SetDebugName("Random-Buffer");
    
    // SceneBuffer
    FBufferParams SceneBufferParams;
    SceneBufferParams.Size             = sizeof(FSceneBuffer);
    SceneBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    SceneBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pSceneBuffer = FBuffer::Create(m_pDevice, SceneBufferParams, m_pDeviceAllocator);
    assert(m_pSceneBuffer != nullptr);
    m_pSceneBuffer->SetDebugName("Scene-Buffer");
    
    // TonemappingBuffer
    FBufferParams TonemappingBufferParams;
    TonemappingBufferParams.Size             = sizeof(FTonemappingBuffer);
    TonemappingBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    TonemappingBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pTonemappingBuffer = FBuffer::Create(m_pDevice, TonemappingBufferParams, m_pDeviceAllocator);
    assert(m_pTonemappingBuffer != nullptr);
    m_pTonemappingBuffer->SetDebugName("Tonemapping-Buffer");
    
    // QuadBuffer
    FBufferParams QuadBufferParams;
    QuadBufferParams.Size             = sizeof(FShaderQuad) * MAX_QUADS;
    QuadBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    QuadBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pQuadBuffer = FBuffer::Create(m_pDevice, QuadBufferParams, m_pDeviceAllocator);
    assert(m_pQuadBuffer != nullptr);
    m_pQuadBuffer->SetDebugName("Quad-Buffer");
    
    // SphereBuffer
    FBufferParams SphereBufferParams;
    SphereBufferParams.Size             = sizeof(FShaderSphere) * MAX_SPHERES;
    SphereBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    SphereBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pSphereBuffer = FBuffer::Create(m_pDevice, SphereBufferParams, m_pDeviceAllocator);
    assert(m_pSphereBuffer != nullptr);
    m_pSphereBuffer->SetDebugName("Sphere-Buffer");
    
    // VertexBuffer
    FBufferParams VertexBufferParams;
    VertexBufferParams.Size             = sizeof(FVertexPosOnly) * MAX_VERTICES;
    VertexBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    VertexBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pVertexBuffer = FBuffer::Create(m_pDevice, VertexBufferParams, m_pDeviceAllocator);
    assert(m_pVertexBuffer != nullptr);
    m_pVertexBuffer->SetDebugName("Vertex-Buffer");
    
    VertexBufferParams.Size = sizeof(FVertexEx) * MAX_VERTICES;
    
    m_pVertexExBuffer = FBuffer::Create(m_pDevice, VertexBufferParams, m_pDeviceAllocator);
    assert(m_pVertexExBuffer != nullptr);
    m_pVertexExBuffer->SetDebugName("VertexEx-Buffer");
    
    // TriangleBuffer
    FBufferParams TriangleBufferParams;
    TriangleBufferParams.Size             = sizeof(FShaderTriangle) * MAX_TRIANGLES;
    TriangleBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    TriangleBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pTriangleBuffer = FBuffer::Create(m_pDevice, TriangleBufferParams, m_pDeviceAllocator);
    assert(m_pTriangleBuffer != nullptr);
    m_pTriangleBuffer->SetDebugName("Triangle-Buffer");
    
    // TriangleMeshesBuffer
    FBufferParams MeshBufferParams;
    MeshBufferParams.Size             = sizeof(FShaderMesh) * MAX_TRIANGLEMESHES;
    MeshBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    MeshBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pMeshBuffer = FBuffer::Create(m_pDevice, MeshBufferParams, m_pDeviceAllocator);
    assert(m_pMeshBuffer != nullptr);
    m_pMeshBuffer->SetDebugName("TriangleMeshes-Buffer");
    
    // MaterialBuffer
    FBufferParams MaterialBufferParams;
    MaterialBufferParams.Size             = sizeof(FShaderMaterial) * MAX_MATERIALS;
    MaterialBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    MaterialBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pMaterialBuffer = FBuffer::Create(m_pDevice, MaterialBufferParams, m_pDeviceAllocator);
    assert(m_pMaterialBuffer != nullptr);
    m_pMaterialBuffer->SetDebugName("Material-Buffer");
    
    // BoundingBoxBuffer
    FBufferParams BoundingBoxBufferParams;
    BoundingBoxBufferParams.Size             = sizeof(FShaderBoundingBox) * MAX_BVH_NODES;
    BoundingBoxBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    BoundingBoxBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pBvhBuffer = FBuffer::Create(m_pDevice, BoundingBoxBufferParams, m_pDeviceAllocator);
    assert(m_pBvhBuffer != nullptr);
    m_pBvhBuffer->SetDebugName("BVH-Buffer");
    
    // Debug AABB instance buffer
    FBufferParams AABBInstanceBufferParams;
    AABBInstanceBufferParams.Size             = sizeof(glm::mat4) * MAX_BVH_NODES;
    AABBInstanceBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    AABBInstanceBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pAABBInstanceBuffer = FBuffer::Create(m_pDevice, AABBInstanceBufferParams, m_pDeviceAllocator);
    assert(m_pAABBInstanceBuffer != nullptr);
    m_pAABBInstanceBuffer->SetDebugName("Debug AABB Instance Buffer");
}

void FSoftwareRayTracer::CreateDescriptorSet()
{
    // RayTracing Pass
    m_pRayTracingDescriptorSet0 = FDescriptorSet::Create(m_pDevice, m_pDescriptorPool, m_pRayTracingDescriptorSetLayout);
    assert(m_pRayTracingDescriptorSet0 != nullptr);
    m_pRayTracingDescriptorSet0->SetDebugName("RayTracingPass DescriptorSet0");

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
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pVertexExBuffer->GetBuffer(), 10);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pTriangleBuffer->GetBuffer(), 11);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pMeshBuffer->GetBuffer(), 12);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pBvhBuffer->GetBuffer(), 13);
    
    m_pRayTracingDescriptorSet1 = FDescriptorSet::Create(m_pDevice, m_pDescriptorPool, m_pRayTracingDescriptorSetLayout);
    assert(m_pRayTracingDescriptorSet1 != nullptr);
    m_pRayTracingDescriptorSet1->SetDebugName("RayTracingPass DescriptorSet1");

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
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pVertexExBuffer->GetBuffer(), 10);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pTriangleBuffer->GetBuffer(), 11);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pMeshBuffer->GetBuffer(), 12);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pBvhBuffer->GetBuffer(), 13);
    
    // Tonemapping Pass
    m_pTonemappingDescriptorSet0 = FDescriptorSet::Create(m_pDevice, m_pDescriptorPool, m_pTonemappingDescriptorSetLayout);
    assert(m_pTonemappingDescriptorSet0 != nullptr);
    m_pTonemappingDescriptorSet0->SetDebugName("TonemappingPass DescriptorSet0");

    m_pTonemappingDescriptorSet0->BindCombinedImageSampler(m_pSceneTextureView0->GetImageView(), m_pTonemapSampler->GetSampler(), 0);
    m_pTonemappingDescriptorSet0->BindUniformBuffer(m_pTonemappingBuffer->GetBuffer(), 1);
    
    m_pTonemappingDescriptorSet1 = FDescriptorSet::Create(m_pDevice, m_pDescriptorPool, m_pTonemappingDescriptorSetLayout);
    assert(m_pTonemappingDescriptorSet1 != nullptr);
    m_pTonemappingDescriptorSet1->SetDebugName("TonemappingPass DescriptorSet1");

    m_pTonemappingDescriptorSet1->BindCombinedImageSampler(m_pSceneTextureView1->GetImageView(), m_pTonemapSampler->GetSampler(), 0);
    m_pTonemappingDescriptorSet1->BindUniformBuffer(m_pTonemappingBuffer->GetBuffer(), 1);
    
    // Debug Pass
    m_pDebugDescriptorSet0 = FDescriptorSet::Create(m_pDevice, m_pDescriptorPool, m_pDebugDescriptorSetLayout);
    assert(m_pDebugDescriptorSet0 != nullptr);
    m_pDebugDescriptorSet0->SetDebugName("DebugPass DescriptorSet0");

    m_pDebugDescriptorSet0->BindUniformBuffer(m_pCameraBuffer->GetBuffer(), 0);
    m_pDebugDescriptorSet0->BindStorageBuffer(m_pAABBInstanceBuffer->GetBuffer(), 1);
    
    m_pDebugDescriptorSet1 = FDescriptorSet::Create(m_pDevice, m_pDescriptorPool, m_pDebugDescriptorSetLayout);
    assert(m_pDebugDescriptorSet1 != nullptr);
    m_pDebugDescriptorSet1->SetDebugName("DebugPass DescriptorSet1");

    m_pDebugDescriptorSet1->BindUniformBuffer(m_pCameraBuffer->GetBuffer(), 0);
    m_pDebugDescriptorSet1->BindStorageBuffer(m_pAABBInstanceBuffer->GetBuffer(), 1);
}

void FSoftwareRayTracer::ReleaseDescriptorSets()
{
    SAFE_DELETE(m_pRayTracingDescriptorSet0);
    SAFE_DELETE(m_pRayTracingDescriptorSet1);
    SAFE_DELETE(m_pTonemappingDescriptorSet0);
    SAFE_DELETE(m_pTonemappingDescriptorSet1);
    SAFE_DELETE(m_pDebugDescriptorSet0);
    SAFE_DELETE(m_pDebugDescriptorSet1);
    SAFE_DELETE(m_pOutputTextureDescriptorSet);
}

void FSoftwareRayTracer::CreateOrResizeSceneTexture(uint32_t Width, uint32_t Height)
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
        SAFE_DELETE(m_pDepthBufferTexture);
        SAFE_DELETE(m_pDepthBufferTextureView);
        SAFE_DELETE(m_pTonemappingFramebuffer);
        SAFE_DELETE(m_pDebugFramebuffer);
        
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
    m_pSceneTexture0->SetDebugName("SceneTexture0");

    {
        FTextureViewParams TextureViewParams = {};
        TextureViewParams.pTexture = m_pSceneTexture0;

        m_pSceneTextureView0 = FTextureView::Create(m_pDevice, TextureViewParams);
        assert(m_pSceneTextureView0 != nullptr);
        m_pSceneTextureView0->SetDebugName("SceneTextureView0");
    }

    // Scene texture frame 1
    m_pSceneTexture1 = FTexture::Create(m_pDevice, TextureParams);
    assert(m_pSceneTexture1 != nullptr);
    m_pSceneTexture1->SetDebugName("SceneTexture1");
    
    {
        FTextureViewParams TextureViewParams = {};
        TextureViewParams.pTexture = m_pSceneTexture1;
        
        m_pSceneTextureView1 = FTextureView::Create(m_pDevice, TextureViewParams);
        assert(m_pSceneTextureView1 != nullptr);
        m_pSceneTextureView1->SetDebugName("SceneTextureView1");
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
    m_pOutputTexture->SetDebugName("OutputTexture");
    
    {
        FTextureViewParams TextureViewParams = {};
        TextureViewParams.pTexture = m_pOutputTexture;
        
        m_pOutputTextureView = FTextureView::Create(m_pDevice, TextureViewParams);
        assert(m_pOutputTextureView != nullptr);
        m_pOutputTextureView->SetDebugName("OutputTextureView");
    }
    
    // Create depth-buffer texture for the viewport
    FTextureParams DepthBufferParams = {};
    DepthBufferParams.Format        = VK_FORMAT_D24_UNORM_S8_UINT;
    DepthBufferParams.ImageType     = VK_IMAGE_TYPE_2D;
    DepthBufferParams.Width         = m_ViewportWidth  = Width;
    DepthBufferParams.Height        = m_ViewportHeight = Height;
    DepthBufferParams.Usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    DepthBufferParams.InitialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    m_pDepthBufferTexture = FTexture::Create(m_pDevice, DepthBufferParams);
    assert(m_pDepthBufferTexture != nullptr);
    m_pDepthBufferTexture->SetDebugName("DepthBuffer");
    
    {
        FTextureViewParams TextureViewParams = {};
        TextureViewParams.pTexture = m_pDepthBufferTexture;
        
        m_pDepthBufferTextureView = FTextureView::Create(m_pDevice, TextureViewParams);
        assert(m_pDepthBufferTextureView != nullptr);
        m_pDepthBufferTextureView->SetDebugName("DepthBufferView");
    }
    
    // Create Framebuffer for the tonemap stage
    VkImageView ImageView = m_pOutputTextureView->GetImageView();
    
    FFramebufferParams FramebufferParams = {};
    FramebufferParams.AttachmentCount = 1;
    FramebufferParams.Width           = m_ViewportWidth;
    FramebufferParams.Height          = m_ViewportHeight;
    FramebufferParams.pRenderPass     = m_pTonemappingRenderPass;
    FramebufferParams.pAttachMents    = &ImageView;

    m_pTonemappingFramebuffer = FFramebuffer::Create(m_pDevice, FramebufferParams);
    m_pTonemappingFramebuffer->SetDebugName("TonemappingPass FrameBuffer");

    // Create Framebuffer for the tonemap stage
    VkImageView DebugPassImageViews[] =
    {
        m_pOutputTextureView->GetImageView(),
        m_pDepthBufferTextureView->GetImageView()
    };
    
    FramebufferParams.AttachmentCount = 2;
    FramebufferParams.pRenderPass     = m_pDebugRenderPass;
    FramebufferParams.pAttachMents    = DebugPassImageViews;

    m_pDebugFramebuffer = FFramebuffer::Create(m_pDevice, FramebufferParams);
    m_pDebugFramebuffer->SetDebugName("DebugPass FrameBuffer");

    // UI DescriptorSet
    m_pOutputTextureDescriptorSet = GUI::AllocateTextureID(m_pOutputTextureView);
    assert(m_pOutputTextureDescriptorSet != nullptr);

    // Descriptor set for when tracing
    CreateDescriptorSet();

    // When we have resized we need to clear the image as well
    m_bResetImage = true;
}

void FSoftwareRayTracer::ReloadShader()
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
                LOG("FAILED to Compile Shaders\n");
                bIsCompiling = false;
                return false;
            }

            // Upload the new shaders
            LOG("Compiled Shaders Successfully\n");

            // Create shader and pipeline
            FShaderModule* pComputeShader = FShaderModule::CreateFromFile(m_pDevice, "main", RESOURCE_PATH"/shaders/raytracer.spv");
            if (!pComputeShader)
            {
                LOG("FAILED to create ComputeShader\n");
                bIsCompiling = false;
                return false;
            }

            FComputePipelineStateParams pipelineParams = {};
            pipelineParams.pShader         = pComputeShader;
            pipelineParams.pPipelineLayout = m_pRayTracingPipelineLayout;

            FComputePipeline* pComputePipeline  = FComputePipeline::Create(m_pDevice, pipelineParams);
            if (!pComputePipeline)
            {
                LOG("FAILED to create ComputePipeline\n");
                SAFE_DELETE(pComputeShader);
                bIsCompiling = false;
                return false;
            }
            else
            {
                pComputePipeline->SetDebugName("RayTracingPass Pipeline");
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

void FSoftwareRayTracer::UpdateGlobalBuffers(FCommandBuffer* pCommandBuffer)
{
    // Update GPU buffers
    if (m_pScene->m_bUpdateBuffers && m_pScene->m_pVertexBuffer)
    {
        VkBufferCopy BufferCopy;
        BufferCopy.size      = m_pScene->m_pVertexBuffer->GetSize();
        BufferCopy.dstOffset = 0;
        BufferCopy.srcOffset = 0;
        
        pCommandBuffer->CopyBuffer(m_pScene->m_pVertexBuffer->GetBuffer(), m_pVertexBuffer->GetBuffer(), 1, &BufferCopy);
    }
    
    if (m_pScene->m_bUpdateBuffers && m_pScene->m_pVertexExBuffer)
    {
        VkBufferCopy BufferCopy;
        BufferCopy.size      = m_pScene->m_pVertexExBuffer->GetSize();
        BufferCopy.dstOffset = 0;
        BufferCopy.srcOffset = 0;
        
        pCommandBuffer->CopyBuffer(m_pScene->m_pVertexExBuffer->GetBuffer(), m_pVertexExBuffer->GetBuffer(), 1, &BufferCopy);
    }
    
    if (m_pScene->m_bUpdateBuffers && m_pScene->m_pTriangleBuffer)
    {
        VkBufferCopy BufferCopy;
        BufferCopy.size      = m_pScene->m_pTriangleBuffer->GetSize();
        BufferCopy.dstOffset = 0;
        BufferCopy.srcOffset = 0;
        
        pCommandBuffer->CopyBuffer(m_pScene->m_pTriangleBuffer->GetBuffer(), m_pTriangleBuffer->GetBuffer(), 1, &BufferCopy);
    }
    
    if (m_pScene->m_bUpdateBuffers && m_pScene->m_pBoundingBoxBuffer)
    {
        VkBufferCopy BufferCopy;
        BufferCopy.size      = m_pScene->m_pBoundingBoxBuffer->GetSize();
        BufferCopy.dstOffset = 0;
        BufferCopy.srcOffset = 0;
        
        pCommandBuffer->CopyBuffer(m_pScene->m_pBoundingBoxBuffer->GetBuffer(), m_pBvhBuffer->GetBuffer(), 1, &BufferCopy);
    }
    
    if (m_pScene->m_bUpdateBuffers && m_pScene->m_pAABBInstanceBuffer)
    {
        VkBufferCopy BufferCopy;
        BufferCopy.size      = m_pScene->m_pAABBInstanceBuffer->GetSize();
        BufferCopy.dstOffset = 0;
        BufferCopy.srcOffset = 0;
        
        pCommandBuffer->CopyBuffer(m_pScene->m_pAABBInstanceBuffer->GetBuffer(), m_pAABBInstanceBuffer->GetBuffer(), 1, &BufferCopy);
    }
    
    // Do not update next frame
    m_pScene->m_bUpdateBuffers = false;
    
    // Update smaller buffers
    if (!m_pScene->m_Quads.empty())
    {
        pCommandBuffer->FillBuffer(m_pQuadBuffer, 0, m_pQuadBuffer->GetSize(), 0);
        assert(sizeof(FShaderQuad) * m_pScene->m_Quads.size() < m_pQuadBuffer->GetSize());
        pCommandBuffer->UpdateBuffer(m_pQuadBuffer, 0, sizeof(FShaderQuad) * m_pScene->m_Quads.size(), m_pScene->m_Quads.data());
    }
    
    if (!m_pScene->m_Spheres.empty())
    {
        pCommandBuffer->FillBuffer(m_pSphereBuffer, 0, m_pSphereBuffer->GetSize(), 0);
        assert(sizeof(FShaderSphere) * m_pScene->m_Spheres.size() < m_pSphereBuffer->GetSize());
        pCommandBuffer->UpdateBuffer(m_pSphereBuffer, 0, sizeof(FShaderSphere) * m_pScene->m_Spheres.size(), m_pScene->m_Spheres.data());
    }
    
    if (!m_pScene->m_Meshes.empty())
    {
        pCommandBuffer->FillBuffer(m_pMeshBuffer, 0, m_pMeshBuffer->GetSize(), 0);
        assert(sizeof(FShaderMesh) * m_pScene->m_Meshes.size() < m_pMeshBuffer->GetSize());
        pCommandBuffer->UpdateBuffer(m_pMeshBuffer, 0, sizeof(FShaderMesh) * m_pScene->m_Meshes.size(), m_pScene->m_Meshes.data());
    }
    
    if (!m_pScene->m_GpuMaterials.empty())
    {
        pCommandBuffer->FillBuffer(m_pMaterialBuffer, 0, m_pMaterialBuffer->GetSize(), 0);
        assert(sizeof(FShaderMaterial) * m_pScene->m_GpuMaterials.size() < m_pMaterialBuffer->GetSize());
        pCommandBuffer->UpdateBuffer(m_pMaterialBuffer, 0, sizeof(FShaderMaterial) * m_pScene->m_GpuMaterials.size(), m_pScene->m_GpuMaterials.data());
    }
}
