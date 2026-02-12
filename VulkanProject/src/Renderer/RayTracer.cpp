#include "RayTracer.h"
#include "GUI.h"
#include "Application.h"
#include "TextureResource.h"
#include "Vulkan/Device.h"
#include "Vulkan/PipelineState.h"
#include "Vulkan/ShaderModule.h"
#include "Vulkan/DescriptorSetLayout.h"
#include "Vulkan/PipelineLayout.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/Texture.h"
#include "Vulkan/TextureView.h"
#include "Vulkan/BindlessManager.h"

CRayTracer::CRayTracer()
    : CBaseRenderer()
    , m_pScene(nullptr)
    , m_pSceneSettingsBuffer(nullptr)
    , m_pMaterialBuffer(nullptr)
    , m_pMeshBuffer(nullptr)
    , m_pRayTracingPipeline(nullptr)
    , m_pRayTracingPipelineLayout(nullptr)
    , m_pRayTracingDescriptorSetLayout(nullptr)
    , m_pRayTracingDescriptorSet0(nullptr)
    , m_pRayTracingDescriptorSet1(nullptr)
{
}

CRayTracer::~CRayTracer()
{
}

void CRayTracer::CreateResources()
{
    m_pScene = SceneFactory::CreateScene(ESceneType::Spheres);

    // Create RayTracing PipelineLayout
    SPipelineLayoutParams RayTracingPipelineLayoutParams;
    RayTracingPipelineLayoutParams.ppLayouts       = nullptr;
    RayTracingPipelineLayoutParams.NumLayouts      = 0;
    RayTracingPipelineLayoutParams.bEnableBindless = true;
    
    m_pRayTracingPipelineLayout = CPipelineLayout::Create(GetDevice(), RayTracingPipelineLayoutParams);
    assert(m_pRayTracingPipelineLayout != nullptr);
    m_pRayTracingPipelineLayout->SetDebugName("RayTracingPass PipelineLayout");

    CShaderModule* pRayGenShader = CShaderModule::CreateFromFile(GetDevice(), "main", RESOURCE_PATH"/shaders/compiled_shaders/raygen.spv");
    assert(pRayGenShader != nullptr);
    pRayGenShader->SetDebugName(RESOURCE_PATH"/shaders/compiled_shaders/raygen.spv");

    CShaderModule* pRayMissShader = CShaderModule::CreateFromFile(GetDevice(), "main", RESOURCE_PATH"/shaders/compiled_shaders/miss.spv");
    assert(pRayMissShader != nullptr);
    pRayMissShader->SetDebugName(RESOURCE_PATH"/shaders/compiled_shaders/miss.spv");

    CShaderModule* pRayClosestHitShader = CShaderModule::CreateFromFile(GetDevice(), "main", RESOURCE_PATH"/shaders/compiled_shaders/closesthit.spv");
    assert(pRayClosestHitShader != nullptr);
    pRayClosestHitShader->SetDebugName(RESOURCE_PATH"/shaders/compiled_shaders/closesthit.spv");

    CShaderModule* pRayAnyHitShader = CShaderModule::CreateFromFile(GetDevice(), "main", RESOURCE_PATH"/shaders/compiled_shaders/anyhit.spv");
    assert(pRayAnyHitShader != nullptr);
    pRayAnyHitShader->SetDebugName(RESOURCE_PATH"/shaders/compiled_shaders/anyhit.spv");

    SRayTracingPipelineStateParams PipelineParams;
    PipelineParams.pRayGenShader        = pRayGenShader;
    PipelineParams.pRayMissShader       = pRayMissShader;
    PipelineParams.pRayClosestHitShader = pRayClosestHitShader;
    PipelineParams.pRayAnyHitShader     = pRayAnyHitShader;
    PipelineParams.pPipelineLayout      = m_pRayTracingPipelineLayout;

    m_pRayTracingPipeline = CRayTracingPipeline::Create(GetDevice(), PipelineParams);
    assert(m_pRayTracingPipeline != nullptr);

    SAFE_DELETE(pRayGenShader);
    SAFE_DELETE(pRayMissShader);
    SAFE_DELETE(pRayClosestHitShader);
    SAFE_DELETE(pRayAnyHitShader);
}

void CRayTracer::ReleaseResources()
{
    SAFE_DELETE(m_pScene);

    SAFE_DELETE(m_pSceneSettingsBuffer);
    SAFE_DELETE(m_pMaterialBuffer);
    SAFE_DELETE(m_pMeshBuffer);

    SAFE_DELETE(m_pRayTracingPipeline);
    SAFE_DELETE(m_pRayTracingPipelineLayout);
    SAFE_DELETE(m_pRayTracingDescriptorSetLayout);
    SAFE_DELETE(m_pRayTracingDescriptorSet0);
    SAFE_DELETE(m_pRayTracingDescriptorSet1);
}

void CRayTracer::CreateDescriptorSets()
{
    // Create common DescriptorSets
    CBaseRenderer::CreateDescriptorSets();

    if (!m_pScene)
    {
        return;
    }

    // RayTracing Pass (fully bindless)
    GetDevice()->GetBindlessManager().BindStorageImage(m_pSceneTextureView0->GetImageView(), 2);
    GetDevice()->GetBindlessManager().BindStorageImage(m_pSceneTextureView1->GetImageView(), 3);
    GetDevice()->GetBindlessManager().BindAccelerationStructure(m_pScene->m_pTopLevelAS->GetAccelerationStructure(), 1);
    GetDevice()->GetBindlessManager().BindCombinedImageSampler(m_pSkybox->GetTextureView()->GetImageView(), m_pSkyboxSampler->GetSampler(), 4);
    GetDevice()->GetBindlessManager().BindStorageBuffer(m_pMeshBuffer->GetBuffer(), m_pMeshBuffer->GetSize(), 5);
    GetDevice()->GetBindlessManager().BindStorageBuffer(m_pMaterialBuffer->GetBuffer(), m_pMaterialBuffer->GetSize(), 6);
    GetDevice()->GetBindlessManager().BindUniformBuffer(m_pCameraBuffer->GetBuffer(), m_pCameraBuffer->GetSize(), 14);
    GetDevice()->GetBindlessManager().BindUniformBuffer(m_pRandomBuffer->GetBuffer(), m_pRandomBuffer->GetSize(), 15);
    GetDevice()->GetBindlessManager().BindUniformBuffer(m_pSceneSettingsBuffer->GetBuffer(), m_pSceneSettingsBuffer->GetSize(), 16);
}

void CRayTracer::ReleaseDescriptorSets()
{
    CBaseRenderer::ReleaseDescriptorSets();

    SAFE_DELETE(m_pRayTracingDescriptorSet0);
    SAFE_DELETE(m_pRayTracingDescriptorSet1);
}

void CRayTracer::RenderUI()
{
    if (ImGui::Begin("Scene Inspector"))
    {
        // Select the view-mode
        {
            static const char* ViewModes[] =
            {
                "Render",
                "Normals",
                "Geometric Normals",
                "Tangents",
                "Albedo",
                "Barycentrics",
                "TexCoords",
            };

            static int CurrentViewMode = static_cast<int>(m_pScene->m_Settings.ViewMode);
            static int PrevViewMode = CurrentViewMode;

            ImGui::Text("Renderer View:");
            ImGui::Separator();

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
                    m_pScene->m_Settings.ViewMode = EViewMode::GeometricNormals;
                }
                else if (CurrentViewMode == 3)
                {
                    m_pScene->m_Settings.ViewMode = EViewMode::Tangents;
                }
                else if (CurrentViewMode == 4)
                {
                    m_pScene->m_Settings.ViewMode = EViewMode::Albedo;
                }
                else if (CurrentViewMode == 5)
                {
                    m_pScene->m_Settings.ViewMode = EViewMode::Barycentrics;
                }
                else if (CurrentViewMode == 6)
                {
                    m_pScene->m_Settings.ViewMode = EViewMode::TexCoords;
                }

                PrevViewMode = CurrentViewMode;
                ResetImage();
            }
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
            static int PrevScene = 0;
            ImGui::Combo("Current Scene", &CurrentScene, Scenes, IM_ARRAYSIZE(Scenes));

            if (PrevScene != CurrentScene)
            {
                CDevice* pDevice = CApplication::Get().GetDevice();
                pDevice->WaitForIdle();

                // Release Descriptors
                ReleaseDescriptorSets();

                const EViewMode ViewMode = m_pScene->m_Settings.ViewMode;
                if (CurrentScene == 0) // Change to Sphere-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = SceneFactory::CreateScene(ESceneType::Spheres);
                }
                else if (CurrentScene == 1) // Change to CornellBox-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = SceneFactory::CreateScene(ESceneType::CornellBox);
                }
                else if (CurrentScene == 2) // Change to Triangles-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = SceneFactory::CreateScene(ESceneType::Triangles);
                }
                else if (CurrentScene == 3) // Change to Sponza-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = SceneFactory::CreateScene(ESceneType::Sponza);
                }
                else if (CurrentScene == 4) // Change to "Polished Glass Sphere"-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = SceneFactory::CreateScene(ESceneType::PolishedGlassSpheres);
                }
                else if (CurrentScene == 5) // Change to "Rough Colored Glass Spheres"-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = SceneFactory::CreateScene(ESceneType::RoughColoredGlassSpheres);
                }
                else if (CurrentScene == 6) // Change to "Rough Transparent Glass Spheres"-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = SceneFactory::CreateScene(ESceneType::RoughTransparentGlassSpheres);
                }

                assert(m_pScene != nullptr);
                ResetImage();

                m_pScene->m_Settings.ViewMode = ViewMode;
                PrevScene = CurrentScene;

                // Create Descriptors
                CreateDescriptorSets();
            }
        }

        // Background
        static const char* Background[] =
        {
            "None",
            "Gradient",
            "Skybox",
        };

        int CurrentBG = static_cast<int>(m_pScene->m_Settings.BackgroundType);

        static int PrevBG = CurrentBG;
        ImGui::Combo("Background", &CurrentBG, Background, IM_ARRAYSIZE(Background));

        if (PrevBG != CurrentBG)
        {
            if (CurrentBG == 0)
            {
                m_pScene->m_Settings.BackgroundType = EBackgroundType::None;
            }
            else if (CurrentBG == 1)
            {
                m_pScene->m_Settings.BackgroundType = EBackgroundType::Gradient;
            }
            else if (CurrentBG == 2)
            {
                m_pScene->m_Settings.BackgroundType = EBackgroundType::Skybox;
            }

            ResetImage();
            PrevBG = CurrentBG;
        }

        if (CurrentBG == 1)
        {
            float Strength = m_pScene->m_Settings.GradientLightStrength;
            if (ImGui::DragFloat("Gradient Strength", &Strength, 0.1f, 1.0f, 100.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
            {
                m_pScene->m_Settings.GradientLightStrength = Strength;
                ResetImage();
            }
        }

        float Exposure = m_pScene->m_Settings.Exposure;
        if (ImGui::DragFloat("Exposure", &Exposure, 0.01f, 0.0f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
        {
            m_pScene->m_Settings.Exposure = Exposure;
            ResetImage();
        }

        float FieldOfView = m_pScene->m_Settings.FieldOfView;
        if (ImGui::DragFloat("FieldOfView", &FieldOfView, 0.1f, 30.0f, 120.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
        {
            m_pScene->m_Settings.FieldOfView = FieldOfView;
            ResetImage();
        }

        int NumBounces = m_pScene->m_Settings.NumBounces;
        if (ImGui::DragInt("Num Bounces", &NumBounces, 1, 1, 1024, "%d", ImGuiSliderFlags_AlwaysClamp))
        {
            m_pScene->m_Settings.NumBounces = NumBounces;
            ResetImage();
        }

        // Reset the scene
        if (ImGui::Button("Reset Camera"))
        {
            m_pScene->Reset();
            ResetImage();
        }

        ImGui::End();
    }
}

void CRayTracer::ReloadShaders()
{
}

void CRayTracer::Render(CCommandBuffer* pCommandBuffer)
{
    // Update necessary buffers
    UpdateGlobalBuffers(pCommandBuffer);

    // Update Scene
    SSceneBuffer SceneBuffer = {};
    SceneBuffer.NumMaterials          = m_pScene->m_GpuMaterials.size();
    SceneBuffer.BackgroundType        = static_cast<uint32_t>(m_pScene->m_Settings.BackgroundType);
    SceneBuffer.NumBounces            = m_pScene->m_Settings.NumBounces;
    SceneBuffer.ViewMode              = static_cast<uint32_t>(m_pScene->m_Settings.ViewMode);
    SceneBuffer.GradientLightStrength = m_pScene->m_Settings.GradientLightStrength;
    
    pCommandBuffer->UpdateBuffer(m_pSceneSettingsBuffer, 0, sizeof(SSceneBuffer), &SceneBuffer);

    // Barrier before reading the buffer from the shader
    VkMemoryBarrier MemoryBarrier;
    MemoryBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    MemoryBarrier.pNext         = nullptr;
    MemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    MemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    pCommandBuffer->PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 1, &MemoryBarrier, 0, nullptr, 0, nullptr);

    // Perform RayTracing
    pCommandBuffer->BindRayTracingPipelineState(m_pRayTracingPipeline);

    pCommandBuffer->BindBindlessDescriptors(m_pRayTracingPipelineLayout, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR);

    pCommandBuffer->TraceRays(m_pRayTracingPipeline, m_pSceneTexture0->GetWidth(), m_pSceneTexture0->GetHeight(), 1);

    // Scene textures are assumed to be in GENERAL when CBaseRenderer::Render is called
    pCommandBuffer->TransitionImage(m_pSceneTexture0->GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    pCommandBuffer->TransitionImage(m_pSceneTexture1->GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

    // Tonemapping
    PerformTonemapping(pCommandBuffer);

    // Scene textures are assumed to be in GENERAL when CBaseRenderer::Render is called so let's put it back into the correct format
    pCommandBuffer->TransitionImage(m_pSceneTexture0->GetImage(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
    pCommandBuffer->TransitionImage(m_pSceneTexture1->GetImage(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
}

void CRayTracer::CreateGlobalBuffers()
{
    CBaseRenderer::CreateGlobalBuffers();
    const VkBufferUsageFlags DescriptorBufferExtraUsage =
        GetDevice()->IsDescriptorBufferSupported() ? VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT : 0;

    // SceneBuffer
    SBufferParams SceneBufferParams;
    SceneBufferParams.Size             = sizeof(SSceneBuffer);
    SceneBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    SceneBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | DescriptorBufferExtraUsage;

    m_pSceneSettingsBuffer = CBuffer::Create(GetDevice(), SceneBufferParams, nullptr);
    assert(m_pSceneSettingsBuffer != nullptr);
    m_pSceneSettingsBuffer->SetDebugName("SceneBuffer");

    // MeshBuffer
    SBufferParams MeshBufferParams = {};
    MeshBufferParams.Size             = MAX_NUM_MESHES * sizeof(SMeshInfo);
    MeshBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | DescriptorBufferExtraUsage;
    MeshBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;

    m_pMeshBuffer = CBuffer::Create(GetDevice(), MeshBufferParams, nullptr);
    assert(m_pMeshBuffer != nullptr);
    m_pMeshBuffer->SetDebugName("MeshInfoBuffer");

    // MaterialBuffer
    if (!m_pScene->m_GpuMaterials.empty())
    {
        SBufferParams MaterialBufferParams;
        MaterialBufferParams.Size             = MAX_MATERIALS * sizeof(SMaterialHLSL);
        MaterialBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
        MaterialBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | DescriptorBufferExtraUsage;

        m_pMaterialBuffer = CBuffer::Create(GetDevice(), MaterialBufferParams, nullptr);
        assert(m_pMaterialBuffer != nullptr);
        m_pMaterialBuffer->SetDebugName("MaterialBuffer");
    }
}

void CRayTracer::UpdateGlobalBuffers(CCommandBuffer* pCommandBuffer)
{
    // Update GPU buffers
    if (!m_pScene->m_MeshInfoBuffer.empty())
    {
        pCommandBuffer->FillBuffer(m_pMeshBuffer, 0, m_pMeshBuffer->GetSize(), 0);
        assert((sizeof(SMeshInfo) * m_pScene->m_MeshInfoBuffer.size()) <= m_pMeshBuffer->GetSize());
        pCommandBuffer->UpdateBuffer(m_pMeshBuffer, 0, sizeof(SMeshInfo) * m_pScene->m_MeshInfoBuffer.size(), m_pScene->m_MeshInfoBuffer.data());
    }

    if (!m_pScene->m_GpuMaterials.empty())
    {
        pCommandBuffer->FillBuffer(m_pMaterialBuffer, 0, m_pMaterialBuffer->GetSize(), 0);
        assert((sizeof(SMaterialHLSL) * m_pScene->m_GpuMaterials.size()) <= m_pMaterialBuffer->GetSize());
        pCommandBuffer->UpdateBuffer(m_pMaterialBuffer, 0, sizeof(SMaterialHLSL) * m_pScene->m_GpuMaterials.size(), m_pScene->m_GpuMaterials.data());
    }

    // Barrier before reading the buffer from the shader
    VkMemoryBarrier MemoryBarrier;
    MemoryBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    MemoryBarrier.pNext         = nullptr;
    MemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    MemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    pCommandBuffer->PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 1, &MemoryBarrier, 0, nullptr, 0, nullptr);
}
