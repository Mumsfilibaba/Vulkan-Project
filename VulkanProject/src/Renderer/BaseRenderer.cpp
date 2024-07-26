#include "BaseRenderer.h"
#include "GUI.h"
#include "Scene.h"
#include "TextureResource.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/SwapChain.h"

FBaseRenderer::FBaseRenderer()
    : IRenderer()
    , m_pDevice(nullptr)
    , m_pSwapchain(nullptr)
    , m_pDeviceAllocator(nullptr)
    , m_pDescriptorPool(nullptr)
    , m_CommandBuffers()
    , m_TimestampQueries()
{
}

FBaseRenderer::~FBaseRenderer()
{
}

void FBaseRenderer::Init(FDevice* pDevice, FSwapchain* pSwapchain)
{
    // Set device
    m_pDevice    = pDevice;
    m_pSwapchain = pSwapchain;

    // Init the TextureLoader
    FTextureResource::InitLoader(m_pDevice);

    // Create DescriptorPool
    FDescriptorPoolParams DescriptorPoolParams;
    DescriptorPoolParams.NumUniformBuffers        = 128;
    DescriptorPoolParams.NumStorageImages         = 128;
    DescriptorPoolParams.NumStorageBuffers        = 128;
    DescriptorPoolParams.NumCombinedImageSamplers = 128;
    DescriptorPoolParams.MaxSets                  = 16;

    m_pDescriptorPool = FDescriptorPool::Create(m_pDevice, DescriptorPoolParams);
    assert(m_pDescriptorPool != nullptr);
    m_pDescriptorPool->SetDebugName("DescriptorPool");

    // CommandBuffers
    FCommandBufferParams CommandBufferParams = {};
    CommandBufferParams.Level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferParams.QueueType = ECommandQueueType::Graphics;

    uint32_t ImageCount = m_pSwapchain->GetNumBackBuffers();
    m_CommandBuffers.resize(ImageCount);
    for (size_t i = 0; i < m_CommandBuffers.size(); i++)
    {
        FCommandBuffer* pCommandBuffer = FCommandBuffer::Create(m_pDevice, CommandBufferParams);
        assert(pCommandBuffer != nullptr);

        const std::string DebugName = "Frame CommandBuffer[" + std::to_string(i) + "]";
        pCommandBuffer->SetDebugName(DebugName.c_str());

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

        const std::string DebugName = "TimeStamp Queries[" + std::to_string(i) + "]";
        pQuery->SetDebugName(DebugName.c_str());
        
        m_TimestampQueries[i] = pQuery;
    }
    
    // Allocator for GPU memory
    m_pDeviceAllocator = new FDeviceMemoryAllocator(m_pDevice);
}

void FBaseRenderer::Tick(float DeltaTime)
{
}

void FBaseRenderer::OnRenderUI()
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
        
        // Clear the image
        if (ImGui::Button("Clear Image"))
        {
            m_bResetImage = true;
        }

        ImGui::NewLine();

        ImGui::Text("Scene:");
        ImGui::Separator();

        // Render renderer-specific stuff regarding the scene
        RenderSceneUI();

        ImGui::End();
    }
    
    // Viewport
    ImGui::Begin("Viewport");

    m_bViewportHasFocus = ImGui::IsWindowFocused();
    m_ViewportWidth     = ImGui::GetContentRegionAvail().x;
    m_ViewportHeight    = ImGui::GetContentRegionAvail().y;
    
    ImGui::End();
}

void FBaseRenderer::Release()
{
    FTextureResource::ReleaseLoader();

    for (FQuery* Query : m_TimestampQueries)
    {
        SAFE_DELETE(Query);
    }

    for (FCommandBuffer* CommandBuffer : m_CommandBuffers)
    {
        SAFE_DELETE(CommandBuffer);
    }

    m_CommandBuffers.clear();
    m_TimestampQueries.clear();

    SAFE_DELETE(m_pDescriptorPool);
    SAFE_DELETE(m_pDeviceAllocator);
}

void FBaseRenderer::OnWindowResize(uint32_t Width, uint32_t Height)
{
}
