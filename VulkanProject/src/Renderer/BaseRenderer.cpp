#include "BaseRenderer.h"
#include "GUI.h"
#include "TextureResource.h"
#include "Input.h"
#include "Camera.h"
#include "MathHelper.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/SwapChain.h"
#include "Vulkan/BindlessManager.h"
#include "Vulkan/DescriptorSet.h"
#include "Vulkan/DescriptorSetLayout.h"
#include "Vulkan/ShaderModule.h"

CBaseRenderer::CBaseRenderer()
    : IRenderer()
    , m_pDevice(nullptr)
    , m_pSwapchain(nullptr)
    , m_pDeviceAllocator(nullptr)
    , m_pDescriptorPool(nullptr)
    , m_CommandBuffers()
    , m_TimestampQueries()
    , m_pSceneTexture0(nullptr)
    , m_pSceneTextureView0(nullptr)
    , m_pSceneTexture1(nullptr)
    , m_pSceneTextureView1(nullptr)
    , m_pOutputTexture(nullptr)
    , m_pOutputTextureView(nullptr)
    , m_pOutputTextureDescriptorSet(nullptr)
    , m_pSkybox(nullptr)
    , m_SkyboxBindlessIndex(CBindlessManager::InvalidBindlessID)
    , m_pSkyboxSampler(nullptr)
    , m_pTonemapSampler(nullptr)
    , m_pTonemappingPipeline(nullptr)
    , m_pTonemappingRenderPass(nullptr)
    , m_pTonemappingPipelineLayout(nullptr)
    , m_pTonemappingDescriptorSetLayout(nullptr)
    , m_pTonemappingDescriptorSet0(nullptr)
    , m_pTonemappingDescriptorSet1(nullptr)
    , m_pTonemappingFramebuffer(nullptr)
    , m_pCameraBuffer(nullptr)
    , m_pRandomBuffer(nullptr)
    , m_pTonemappingBuffer(nullptr)
    , m_bResetImage(false)
    , m_FrameIndex(0)
    , m_LastCPUTime(0.0f)
    , m_LastGPUTime(0.0f)
    , m_ViewportWidth(0)
    , m_ViewportHeight(0)
    , m_bViewportHasFocus(false)
    , m_bIsRightMouseDown(false)
    , m_LastMousePosition(0.0f)
{
}

CBaseRenderer::~CBaseRenderer()
{
}

void CBaseRenderer::Init(CDevice* pDevice, CSwapchain* pSwapchain)
{
    // Set device
    m_pDevice    = pDevice;
    m_pSwapchain = pSwapchain;

    // Init the TextureLoader
    CTextureResource::InitLoader(m_pDevice);

    // Create DescriptorPool
    SDescriptorPoolParams DescriptorPoolParams;
    DescriptorPoolParams.NumUniformBuffers        = 128;
    DescriptorPoolParams.NumStorageImages         = 128;
    DescriptorPoolParams.NumStorageBuffers        = 128;
    DescriptorPoolParams.NumCombinedImageSamplers = 128;
    DescriptorPoolParams.MaxSets                  = 16;

    m_pDescriptorPool = CDescriptorPool::Create(m_pDevice, DescriptorPoolParams);
    assert(m_pDescriptorPool != nullptr);
    m_pDescriptorPool->SetDebugName("DescriptorPool");

    // CommandBuffers
    SCommandBufferParams CommandBufferParams = {};
    CommandBufferParams.Level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferParams.QueueType = ECommandQueueType::Graphics;

    uint32_t ImageCount = m_pSwapchain->GetNumBackBuffers();
    m_CommandBuffers.resize(ImageCount);
    for (size_t i = 0; i < m_CommandBuffers.size(); i++)
    {
        CCommandBuffer* pCommandBuffer = CCommandBuffer::Create(m_pDevice, CommandBufferParams);
        assert(pCommandBuffer != nullptr);

        const std::string DebugName = "Frame CommandBuffer[" + std::to_string(i) + "]";
        pCommandBuffer->SetDebugName(DebugName.c_str());

        m_CommandBuffers[i] = pCommandBuffer;
    }

    // Timestamp queries
    SQueryParams QueryParams;
    QueryParams.QueryType  = VK_QUERY_TYPE_TIMESTAMP;
    QueryParams.QueryCount = 2;
    
    m_TimestampQueries.resize(ImageCount);
    for (size_t i = 0; i < m_TimestampQueries.size(); i++)
    {
        CQuery* pQuery = CQuery::Create(m_pDevice, QueryParams);
        pQuery->Reset();

        const std::string DebugName = "TimeStamp Queries[" + std::to_string(i) + "]";
        pQuery->SetDebugName(DebugName.c_str());
        
        m_TimestampQueries[i] = pQuery;
    }
    
    // Allocator for GPU memory
    m_pDeviceAllocator = new CDeviceMemoryAllocator(m_pDevice);

    // Create Skybox
    m_pSkybox = CTextureResource::LoadCubeMapFromPanoramaFile(m_pDevice, RESOURCE_PATH"/textures/arches.hdr");
    assert(m_pSkybox != nullptr);

    // Skybox Sampler
    {
        SSamplerParams SamplerParams = {};
        SamplerParams.MagFilter     = VK_FILTER_LINEAR;
        SamplerParams.MinFilter     = VK_FILTER_LINEAR;
        SamplerParams.MipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        SamplerParams.AddressModeU  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SamplerParams.AddressModeV  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SamplerParams.AddressModeW  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        SamplerParams.MinLod        = 0;
        SamplerParams.MaxLod        = 1000;
        SamplerParams.MaxAnisotropy = 1.0f;

        m_pSkyboxSampler = CSampler::Create(pDevice, SamplerParams);
        assert(m_pSkyboxSampler != nullptr);
        m_pSkyboxSampler->SetDebugName("Skybox Sampler");
    }

    // Tonemap Sampler
    {
        SSamplerParams SamplerParams = {};
        SamplerParams.MagFilter     = VK_FILTER_NEAREST;
        SamplerParams.MinFilter     = VK_FILTER_NEAREST;
        SamplerParams.MipmapMode    = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        SamplerParams.AddressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        SamplerParams.AddressModeV  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        SamplerParams.AddressModeW  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        SamplerParams.MinLod        = 0;
        SamplerParams.MaxLod        = 1000;
        SamplerParams.MaxAnisotropy = 1.0f;

        m_pTonemapSampler = CSampler::Create(pDevice, SamplerParams);
        assert(m_pTonemapSampler != nullptr);
        m_pTonemapSampler->SetDebugName("Tonemap Sampler");
    }

    // Bindless Manager
    m_SkyboxBindlessIndex = m_pDevice->GetBindlessManager().AddImageView(m_pSkybox->GetTextureView()->GetImageView(), m_pSkyboxSampler->GetSampler());

    // Init Common Stages
    CreateTonemappingResources();

    // Let the renderer create it's resources
    CreateResources();

    // Create all buffers
    CreateGlobalBuffers();

    // Create the scene texture
    m_ViewportWidth  = 0;
    m_ViewportHeight = 0;
    CreateOrResizeSceneTexture(1280, 720);
}

void CBaseRenderer::CreateTonemappingResources()
{
    // Create Tonemap DescriptorSetLayout
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

    SDescriptorSetLayoutParams TonemappingDescriptorSetLayoutParams;
    TonemappingDescriptorSetLayoutParams.pBindings   = TonemappingBindings;
    TonemappingDescriptorSetLayoutParams.NumBindings = NumTonemappingBindings;

    m_pTonemappingDescriptorSetLayout = CDescriptorSetLayout::Create(GetDevice(), TonemappingDescriptorSetLayoutParams);
    assert(m_pTonemappingDescriptorSetLayout != nullptr);
    m_pTonemappingDescriptorSetLayout->SetDebugName("TonemappingPass DescriptorSetLayout");

    // Create Tonemapping PipelineLayout
    SPipelineLayoutParams ToneMappingPipelineLayoutParams;
    ToneMappingPipelineLayoutParams.ppLayouts  = &m_pTonemappingDescriptorSetLayout;
    ToneMappingPipelineLayoutParams.NumLayouts = 1;

    m_pTonemappingPipelineLayout = CPipelineLayout::Create(GetDevice(), ToneMappingPipelineLayoutParams);
    assert(m_pTonemappingPipelineLayout != nullptr);
    m_pTonemappingPipelineLayout->SetDebugName("TonemappingPass PipelineLayout");

    // PipelineState, RenderPass and Shaders
    CShaderModule* pVertex = CShaderModule::CreateFromFile(GetDevice(), "main", RESOURCE_PATH"/shaders/fullscreenVS.spv");
    assert(pVertex != nullptr);
    pVertex->SetDebugName(RESOURCE_PATH"/shaders/fullscreenVS.spv");

    CShaderModule* pFragment = CShaderModule::CreateFromFile(GetDevice(), "main", RESOURCE_PATH"/shaders/tonemap.spv");
    assert(pFragment != nullptr);
    pFragment->SetDebugName(RESOURCE_PATH"/shaders/tonemap.spv");

    SRenderPassAttachment Attachments[1];
    Attachments[0].Format        = VK_FORMAT_R8G8B8A8_UNORM;
    Attachments[0].InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    Attachments[0].FinalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    SRenderPassParams RenderPassParams = {};
    RenderPassParams.ColorAttachmentCount = 1;
    RenderPassParams.pColorAttachments    = Attachments;

    m_pTonemappingRenderPass = CRenderPass::Create(GetDevice(), RenderPassParams);
    assert(m_pTonemappingRenderPass != nullptr);
    m_pTonemappingRenderPass->SetDebugName("TonemappingPass RenderPass");

    SGraphicsPipelineStateParams TonemappingPipelineParams = {};
    TonemappingPipelineParams.pBindingDescriptions      = nullptr;
    TonemappingPipelineParams.BindingDescriptionCount   = 0;
    TonemappingPipelineParams.pAttributeDescriptions    = nullptr;
    TonemappingPipelineParams.AttributeDescriptionCount = 0;
    TonemappingPipelineParams.pVertexShader             = pVertex;
    TonemappingPipelineParams.pFragmentShader           = pFragment;
    TonemappingPipelineParams.pRenderPass               = m_pTonemappingRenderPass;
    TonemappingPipelineParams.pPipelineLayout           = m_pTonemappingPipelineLayout;

    m_pTonemappingPipeline = CGraphicsPipeline::Create(GetDevice(), TonemappingPipelineParams);
    assert(m_pTonemappingPipeline != nullptr);
    m_pTonemappingPipeline->SetDebugName("TonemappingPass Pipeline");

    delete pVertex;
    delete pFragment;
}

void CBaseRenderer::PerformTonemapping(CCommandBuffer* pCommandBuffer)
{
    // Update Tonemap Settings
    STonemappingBuffer TonemappingBuffer = {};
    TonemappingBuffer.Exposure = GetScene()->GetExposure();
    pCommandBuffer->UpdateBuffer(m_pTonemappingBuffer, 0, sizeof(STonemappingBuffer), &TonemappingBuffer);

    // Begin RenderPass
    VkClearValue ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    pCommandBuffer->BeginRenderPass(m_pTonemappingRenderPass, m_pTonemappingFramebuffer, &ClearColor, 1);

    // Set viewport
    VkViewport Viewport = { 0.0f, 0.0f, float(m_ViewportWidth), float(m_ViewportHeight), 0.0f, 1.0f };
    pCommandBuffer->SetViewport(Viewport);

    VkRect2D scissor = { { 0, 0}, { m_ViewportWidth, m_ViewportHeight } };
    pCommandBuffer->SetScissorRect(scissor);

    // Bind pipeline
    pCommandBuffer->BindGraphicsPipelineState(m_pTonemappingPipeline);

    // Perform Tonemapping
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

    // End RenderPass
    pCommandBuffer->EndRenderPass();
}

void CBaseRenderer::Tick(float DeltaTime)
{
    m_LastCPUTime = DeltaTime * 1000.0f; // deltaTime is in seconds

    // Update scene image
    CreateOrResizeSceneTexture(m_ViewportWidth, m_ViewportHeight);

    // Camera Movement
    const float CameraSpeed = GetScene()->GetCameraSpeed();
    if (m_bViewportHasFocus)
    {
        glm::vec3 Translation(0.0f);
        glm::vec3 Rotation(0.0f);
        if (Input::IsKeyDown(GLFW_KEY_W))
        {
            Translation.z = CameraSpeed * DeltaTime;
        }
        else if (Input::IsKeyDown(GLFW_KEY_S))
        {
            Translation.z = -CameraSpeed * DeltaTime;
        }

        if (Input::IsKeyDown(GLFW_KEY_A))
        {
            Translation.x = CameraSpeed * DeltaTime;
        }
        else if (Input::IsKeyDown(GLFW_KEY_D))
        {
            Translation.x = -CameraSpeed * DeltaTime;
        }

        GetScene()->GetCamera().Move(Translation);

        // Camera rotation with mouse (RMB drag)
        constexpr float CameraMouseSensitivity = 0.0025f;
        if (Input::IsMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
        {
            const glm::vec2 MousePosition = Input::GetMousePosition();
            if (m_bIsRightMouseDown)
            {
                const glm::vec2 MouseDelta = MousePosition - m_LastMousePosition;
                Rotation.x += MouseDelta.y * CameraMouseSensitivity;
                Rotation.y += MouseDelta.x * CameraMouseSensitivity;
            }

            m_LastMousePosition = MousePosition;
            m_bIsRightMouseDown = true;
        }
        else
        {
            m_bIsRightMouseDown = false;
        }

        GetScene()->GetCamera().Rotate(Rotation);

        // Check if we moved and then we reset the image
        if (glm::length(Rotation) > 0.0f || glm::length(Translation) > 0.0f)
        {
            m_bResetImage = true;
        }

        // Reload shaders
        if (Input::IsKeyDown(GLFW_KEY_R))
        {
            ReloadShaders();
        }
    }
    else
    {
        m_bIsRightMouseDown = false;
    }

    // Update
    GetScene()->GetCamera().Update(GetScene()->GetFieldOfView(), m_pSceneTexture0->GetWidth(), m_pSceneTexture0->GetHeight(), 0.01f, 10000.0f);

    // Draw
    uint32_t FrameIndex = m_pSwapchain->GetCurrentBackBufferIndex();
    CQuery* pCurrentTimestampQuery = m_TimestampQueries[FrameIndex];
    CCommandBuffer* pCurrentCommandBuffer = m_CommandBuffers[FrameIndex];

    // Reset CommandBuffer
    pCurrentCommandBuffer->Reset();

    // Prepare timestamps
    constexpr uint32_t TimestampCount = 2;
    uint64_t Timestamps[TimestampCount];
    ZERO_MEMORY(Timestamps, sizeof(uint64_t) * TimestampCount);

    pCurrentTimestampQuery->GetData(0, 2, sizeof(uint64_t) * TimestampCount, &Timestamps, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
    pCurrentTimestampQuery->Reset();

    const double TimestampPeriod = static_cast<double>(m_pDevice->GetTimestampPeriod());
    const double GpuTiming   = (static_cast<double>(Timestamps[1]) - static_cast<double>(Timestamps[0])) * TimestampPeriod;
    const double GpuTimingMS = GpuTiming / 1000000.0;
    m_LastGPUTime = static_cast<float>(GpuTimingMS);

    // Begin CommandBuffer
    pCurrentCommandBuffer->Begin();
    pCurrentCommandBuffer->WriteTimestamp(pCurrentTimestampQuery, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);

    // Clear the image if this is requested
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
    SCameraBuffer CameraBuffer = {};
    CameraBuffer.Projection         = GetScene()->GetCamera().GetProjectionMatrix();
    CameraBuffer.View               = GetScene()->GetCamera().GetViewMatrix();
    CameraBuffer.InverseView        = GetScene()->GetCamera().GetInverseViewMatrix();
    CameraBuffer.InverseProjection  = GetScene()->GetCamera().GetInverseProjectionMatrix();
    CameraBuffer.Position           = glm::vec4(GetScene()->GetCamera().GetPosition(), 0.0f);
    CameraBuffer.Forward            = glm::vec4(GetScene()->GetCamera().GetForward(), 0.0f);
    CameraBuffer.FieldOfViewDegrees = Math::ToDegrees(GetScene()->GetCamera().GetFieldOfView());
    
    pCurrentCommandBuffer->UpdateBuffer(m_pCameraBuffer, 0, sizeof(SCameraBuffer), &CameraBuffer);

    // Update RandomBuffer
    constexpr uint32_t MaxSamples = 16;
    SRandomBuffer RandomBuffer = {};
    RandomBuffer.FrameIndex  = m_FrameIndex;
    RandomBuffer.HaltonIndex = m_FrameIndex % MaxSamples;

    pCurrentCommandBuffer->UpdateBuffer(m_pRandomBuffer, 0, sizeof(SRandomBuffer), &RandomBuffer);

    // Perform the actual rendering
    Render(pCurrentCommandBuffer);

    // End CommandBuffer
    pCurrentCommandBuffer->WriteTimestamp(pCurrentTimestampQuery, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 1);
    pCurrentCommandBuffer->End();

    m_pDevice->ExecuteGraphics(pCurrentCommandBuffer, nullptr, nullptr);
}

void CBaseRenderer::OnRenderUI()
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
    ImGui::Begin("DockSpace", nullptr, WindowFlags);

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

        // Clear the image
        if (ImGui::Button("Clear Image"))
        {
            m_bResetImage = true;
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

    // Renderer Specifics
    RenderUI();
}

void CBaseRenderer::Release()
{
    GetDevice()->WaitForIdle();

    CTextureResource::ReleaseLoader();

    for (CQuery* Query : m_TimestampQueries)
    {
        SAFE_DELETE(Query);
    }

    for (CCommandBuffer* CommandBuffer : m_CommandBuffers)
    {
        SAFE_DELETE(CommandBuffer);
    }

    m_CommandBuffers.clear();
    m_TimestampQueries.clear();

    SAFE_DELETE(m_pOutputTextureDescriptorSet);

    // Release DescriptorSets
    ReleaseDescriptorSets();

    // Release Renderer Specific resources
    ReleaseResources();

    SAFE_DELETE(m_pSkybox);

    SAFE_DELETE(m_pSkyboxSampler);
    SAFE_DELETE(m_pTonemapSampler);
    SAFE_DELETE(m_pSceneTexture1);
    SAFE_DELETE(m_pSceneTextureView1);
    SAFE_DELETE(m_pSceneTexture0);
    SAFE_DELETE(m_pSceneTextureView0);
    SAFE_DELETE(m_pOutputTexture);
    SAFE_DELETE(m_pOutputTextureView);

    SAFE_DELETE(m_pTonemappingRenderPass);
    SAFE_DELETE(m_pTonemappingPipeline);
    SAFE_DELETE(m_pTonemappingPipelineLayout);
    SAFE_DELETE(m_pTonemappingDescriptorSetLayout);
    SAFE_DELETE(m_pTonemappingFramebuffer);

    SAFE_DELETE(m_pCameraBuffer);
    SAFE_DELETE(m_pRandomBuffer);
    SAFE_DELETE(m_pTonemappingBuffer);

    SAFE_DELETE(m_pDescriptorPool);
    SAFE_DELETE(m_pDeviceAllocator);
}

bool CBaseRenderer::CreateOrResizeSceneTexture(uint32_t Width, uint32_t Height)
{
    // If the SceneTexture does not exist yet, we just create the resource directly
    if (m_pSceneTexture0)
    {
        if ((m_pSceneTexture0->GetWidth() == Width && m_pSceneTexture0->GetHeight() == Height) || Width == 0 || Height == 0)
        {
            return false;
        }

        GetDevice()->WaitForIdle();

        SAFE_DELETE(m_pSceneTexture0);
        SAFE_DELETE(m_pSceneTextureView0);
        SAFE_DELETE(m_pSceneTexture1);
        SAFE_DELETE(m_pSceneTextureView1);
        SAFE_DELETE(m_pOutputTexture);
        SAFE_DELETE(m_pOutputTextureView);
        SAFE_DELETE(m_pTonemappingFramebuffer);
        SAFE_DELETE(m_pOutputTextureDescriptorSet);

        ReleaseDescriptorSets();
    }

    // Update the new viewport width and height
    m_ViewportWidth  = Width;
    m_ViewportHeight = Height;

    // Create texture for the viewport
    STextureParams TextureParams = {};
    TextureParams.Format        = VK_FORMAT_R32G32B32A32_SFLOAT;
    TextureParams.ImageType     = VK_IMAGE_TYPE_2D;
    TextureParams.Width         = m_ViewportWidth;
    TextureParams.Height        = m_ViewportHeight;
    TextureParams.Usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    TextureParams.InitialLayout = VK_IMAGE_LAYOUT_GENERAL;

    // Scene texture frame 0
    m_pSceneTexture0 = CTexture::Create(m_pDevice, TextureParams);
    assert(m_pSceneTexture0 != nullptr);
    m_pSceneTexture0->SetDebugName("SceneTexture0");

    {
        STextureViewParams TextureViewParams = {};
        TextureViewParams.pTexture = m_pSceneTexture0;

        m_pSceneTextureView0 = CTextureView::Create(m_pDevice, TextureViewParams);
        assert(m_pSceneTextureView0 != nullptr);
        m_pSceneTextureView0->SetDebugName("SceneTextureView0");
    }

    // Scene texture frame 1
    m_pSceneTexture1 = CTexture::Create(m_pDevice, TextureParams);
    assert(m_pSceneTexture1 != nullptr);
    m_pSceneTexture1->SetDebugName("SceneTexture1");

    {
        STextureViewParams TextureViewParams = {};
        TextureViewParams.pTexture = m_pSceneTexture1;

        m_pSceneTextureView1 = CTextureView::Create(m_pDevice, TextureViewParams);
        assert(m_pSceneTextureView1 != nullptr);
        m_pSceneTextureView1->SetDebugName("SceneTextureView1");
    }

    // Create texture for the viewport
    STextureParams OutputTextureParams = {};
    OutputTextureParams.Format        = VK_FORMAT_R8G8B8A8_UNORM;
    OutputTextureParams.ImageType     = VK_IMAGE_TYPE_2D;
    OutputTextureParams.Width         = m_ViewportWidth;
    OutputTextureParams.Height        = m_ViewportHeight;
    OutputTextureParams.Usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    OutputTextureParams.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    m_pOutputTexture = CTexture::Create(m_pDevice, OutputTextureParams);
    assert(m_pOutputTexture != nullptr);
    m_pOutputTexture->SetDebugName("OutputTexture");

    {
        STextureViewParams TextureViewParams = {};
        TextureViewParams.pTexture = m_pOutputTexture;

        m_pOutputTextureView = CTextureView::Create(m_pDevice, TextureViewParams);
        assert(m_pOutputTextureView != nullptr);
        m_pOutputTextureView->SetDebugName("OutputTextureView");
    }

    // Create Framebuffer for the tonemap stage
    VkImageView ImageView = m_pOutputTextureView->GetImageView();

    SFramebufferParams FramebufferParams = {};
    FramebufferParams.AttachmentCount = 1;
    FramebufferParams.Width           = m_ViewportWidth;
    FramebufferParams.Height          = m_ViewportHeight;
    FramebufferParams.pRenderPass     = m_pTonemappingRenderPass;
    FramebufferParams.pAttachMents    = &ImageView;

    m_pTonemappingFramebuffer = CFramebuffer::Create(m_pDevice, FramebufferParams);
    m_pTonemappingFramebuffer->SetDebugName("TonemappingPass FrameBuffer");

    // UI DescriptorSet
    m_pOutputTextureDescriptorSet = GUI::AllocateTextureID(m_pOutputTextureView);
    assert(m_pOutputTextureDescriptorSet != nullptr);

    // Descriptor set for when tracing
    CreateDescriptorSets();

    // When we have resized we need to clear the image as well
    m_bResetImage = true;

    // Returns true if we resized
    return true;
}

void CBaseRenderer::CreateDescriptorSets()
{
    // Tonemap Pass
    m_pTonemappingDescriptorSet0 = CDescriptorSet::Create(GetDevice(), GetDescriptorPool(), m_pTonemappingDescriptorSetLayout);
    assert(m_pTonemappingDescriptorSet0 != nullptr);
    m_pTonemappingDescriptorSet0->SetDebugName("TonemappingPass DescriptorSet0");

    m_pTonemappingDescriptorSet0->BindCombinedImageSampler(m_pSceneTextureView0->GetImageView(), m_pTonemapSampler->GetSampler(), 0);
    m_pTonemappingDescriptorSet0->BindUniformBuffer(m_pTonemappingBuffer->GetBuffer(), 1);
    
    m_pTonemappingDescriptorSet1 = CDescriptorSet::Create(GetDevice(), GetDescriptorPool(), m_pTonemappingDescriptorSetLayout);
    assert(m_pTonemappingDescriptorSet1 != nullptr);
    m_pTonemappingDescriptorSet1->SetDebugName("TonemappingPass DescriptorSet1");

    m_pTonemappingDescriptorSet1->BindCombinedImageSampler(m_pSceneTextureView1->GetImageView(), m_pTonemapSampler->GetSampler(), 0);
    m_pTonemappingDescriptorSet1->BindUniformBuffer(m_pTonemappingBuffer->GetBuffer(), 1);
}

void CBaseRenderer::ReleaseDescriptorSets()
{
    SAFE_DELETE(m_pTonemappingDescriptorSet0);
    SAFE_DELETE(m_pTonemappingDescriptorSet1);
}

void CBaseRenderer::CreateGlobalBuffers()
{
    // Camera
    SBufferParams CameraBufferParams;
    CameraBufferParams.Size             = sizeof(SCameraBuffer);
    CameraBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    CameraBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pCameraBuffer = CBuffer::Create(GetDevice(), CameraBufferParams, GetDeviceAllocator());
    assert(m_pCameraBuffer != nullptr);
    m_pCameraBuffer->SetDebugName("CameraBuffer");

    // Random
    SBufferParams RandomBufferParams;
    RandomBufferParams.Size             = sizeof(SRandomBuffer);
    RandomBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    RandomBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pRandomBuffer = CBuffer::Create(GetDevice(), RandomBufferParams, GetDeviceAllocator());
    assert(m_pRandomBuffer != nullptr);
    m_pRandomBuffer->SetDebugName("RandomBuffer");

    // TonemappingBuffer
    SBufferParams TonemappingBufferParams;
    TonemappingBufferParams.Size             = sizeof(STonemappingBuffer);
    TonemappingBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    TonemappingBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pTonemappingBuffer = CBuffer::Create(GetDevice(), TonemappingBufferParams, GetDeviceAllocator());
    assert(m_pTonemappingBuffer != nullptr);
    m_pTonemappingBuffer->SetDebugName("TonemappingBuffer");
}