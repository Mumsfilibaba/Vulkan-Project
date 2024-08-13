#include "RayTracer.h"
#include "GUI.h"
#include "Application.h"
#include "Vulkan/Device.h"
#include "Vulkan/PipelineState.h"
#include "Vulkan/ShaderModule.h"
#include "Vulkan/DescriptorSetLayout.h"
#include "Vulkan/PipelineLayout.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/Texture.h"
#include "Vulkan/TextureView.h"

FRayTracer::FRayTracer()
    : FBaseRenderer()
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

FRayTracer::~FRayTracer()
{
}

void FRayTracer::CreateResources()
{
    m_pScene = FSceneFactory::CreateScene(ESceneType::Spheres);

    // Create RayTracing DescriptorSetLayout
    constexpr uint32_t NumRayTracingBindings = 8;
    VkDescriptorSetLayoutBinding RayTracingBindings[NumRayTracingBindings];
    
    // Acceleration structure
    RayTracingBindings[0].binding            = 0;
    RayTracingBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    RayTracingBindings[0].descriptorCount    = 1;
    RayTracingBindings[0].stageFlags         = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    RayTracingBindings[0].pImmutableSamplers = nullptr;

    // Output Image
    RayTracingBindings[1].binding            = 1;
    RayTracingBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    RayTracingBindings[1].descriptorCount    = 1;
    RayTracingBindings[1].stageFlags         = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    RayTracingBindings[1].pImmutableSamplers = nullptr;

    // Previous Image
    RayTracingBindings[2].binding            = 2;
    RayTracingBindings[2].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    RayTracingBindings[2].descriptorCount    = 1;
    RayTracingBindings[2].stageFlags         = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    RayTracingBindings[2].pImmutableSamplers = nullptr;
    
    // CameraBuffer
    RayTracingBindings[3].binding            = 3;
    RayTracingBindings[3].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    RayTracingBindings[3].descriptorCount    = 1;
    RayTracingBindings[3].stageFlags         = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
    RayTracingBindings[3].pImmutableSamplers = nullptr;

    // RandomBuffer
    RayTracingBindings[4].binding            = 4;
    RayTracingBindings[4].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    RayTracingBindings[4].descriptorCount    = 1;
    RayTracingBindings[4].stageFlags         = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
    RayTracingBindings[4].pImmutableSamplers = nullptr;

    // SceneSettingsBuffer
    RayTracingBindings[5].binding            = 5;
    RayTracingBindings[5].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    RayTracingBindings[5].descriptorCount    = 1;
    RayTracingBindings[5].stageFlags         = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;
    RayTracingBindings[5].pImmutableSamplers = nullptr;

    // MeshBuffer
    RayTracingBindings[6].binding            = 6;
    RayTracingBindings[6].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[6].descriptorCount    = 1;
    RayTracingBindings[6].stageFlags         = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
    RayTracingBindings[6].pImmutableSamplers = nullptr;

    // MaterialBuffer
    RayTracingBindings[7].binding            = 7;
    RayTracingBindings[7].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[7].descriptorCount    = 1;
    RayTracingBindings[7].stageFlags         = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
    RayTracingBindings[7].pImmutableSamplers = nullptr;

    FDescriptorSetLayoutParams RayTracingDescriptorSetLayoutParams;
    RayTracingDescriptorSetLayoutParams.pBindings   = RayTracingBindings;
    RayTracingDescriptorSetLayoutParams.NumBindings = NumRayTracingBindings;

    m_pRayTracingDescriptorSetLayout = FDescriptorSetLayout::Create(GetDevice(), RayTracingDescriptorSetLayoutParams);
    assert(m_pRayTracingDescriptorSetLayout != nullptr);
    m_pRayTracingDescriptorSetLayout->SetDebugName("RayTracingPass DescriptorSetLayout");

    // Create RayTracing PipelineLayout
    FPipelineLayoutParams RayTracingPipelineLayoutParams;
    RayTracingPipelineLayoutParams.ppLayouts       = &m_pRayTracingDescriptorSetLayout;
    RayTracingPipelineLayoutParams.NumLayouts      = 1;
    RayTracingPipelineLayoutParams.bEnableBindless = true;
    
    m_pRayTracingPipelineLayout = FPipelineLayout::Create(GetDevice(), RayTracingPipelineLayoutParams);
    assert(m_pRayTracingPipelineLayout != nullptr);
    m_pRayTracingPipelineLayout->SetDebugName("RayTracingPass PipelineLayout");

    FShaderModule* pRayGenShader = FShaderModule::CreateFromFile(GetDevice(), "main", RESOURCE_PATH"/shaders/raygen.spv");
    assert(pRayGenShader != nullptr);
    pRayGenShader->SetDebugName(RESOURCE_PATH"/shaders/raygen.spv");

    FShaderModule* pRayMissShader = FShaderModule::CreateFromFile(GetDevice(), "main", RESOURCE_PATH"/shaders/miss.spv");
    assert(pRayMissShader != nullptr);
    pRayMissShader->SetDebugName(RESOURCE_PATH"/shaders/miss.spv");

    FShaderModule* pRayClosestHitShader = FShaderModule::CreateFromFile(GetDevice(), "main", RESOURCE_PATH"/shaders/closesthit.spv");
    assert(pRayClosestHitShader != nullptr);
    pRayClosestHitShader->SetDebugName(RESOURCE_PATH"/shaders/closesthit.spv");

    FShaderModule* pRayAnyHitShader = FShaderModule::CreateFromFile(GetDevice(), "main", RESOURCE_PATH"/shaders/anyhit.spv");
    assert(pRayAnyHitShader != nullptr);
    pRayAnyHitShader->SetDebugName(RESOURCE_PATH"/shaders/anyhit.spv");

    FRayTracingPipelineStateParams PipelineParams;
    PipelineParams.pRayGenShader        = pRayGenShader;
    PipelineParams.pRayMissShader       = pRayMissShader;
    PipelineParams.pRayClosestHitShader = pRayClosestHitShader;
    PipelineParams.pRayAnyHitShader     = pRayAnyHitShader;
    PipelineParams.pPipelineLayout      = m_pRayTracingPipelineLayout;

    m_pRayTracingPipeline = FRayTracingPipeline::Create(GetDevice(), PipelineParams);
    assert(m_pRayTracingPipeline != nullptr);

    SAFE_DELETE(pRayGenShader);
    SAFE_DELETE(pRayMissShader);
    SAFE_DELETE(pRayClosestHitShader);
    SAFE_DELETE(pRayAnyHitShader);
}

void FRayTracer::ReleaseResources()
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

void FRayTracer::CreateDescriptorSets()
{
    // Create common DescriptorSets
    FBaseRenderer::CreateDescriptorSets();

    if (!m_pScene)
    {
        return;
    }

    // RayTracing Pass
    m_pRayTracingDescriptorSet0 = FDescriptorSet::Create(GetDevice(), GetDescriptorPool(), m_pRayTracingDescriptorSetLayout);
    assert(m_pRayTracingDescriptorSet0 != nullptr);
    m_pRayTracingDescriptorSet0->SetDebugName("RayTracingPass DescriptorSet0");

    m_pRayTracingDescriptorSet0->BindAccelerationStructure(m_pScene->m_pTopLevelAS->GetAccelerationStructure(), 0);
    m_pRayTracingDescriptorSet0->BindStorageImage(m_pSceneTextureView0->GetImageView(), 1);
    m_pRayTracingDescriptorSet0->BindStorageImage(m_pSceneTextureView1->GetImageView(), 2);
    m_pRayTracingDescriptorSet0->BindUniformBuffer(m_pCameraBuffer->GetBuffer(), 3);
    m_pRayTracingDescriptorSet0->BindUniformBuffer(m_pRandomBuffer->GetBuffer(), 4);
    m_pRayTracingDescriptorSet0->BindUniformBuffer(m_pSceneSettingsBuffer->GetBuffer(), 5);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pMeshBuffer->GetBuffer(), 6);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pMaterialBuffer->GetBuffer(), 7);

    m_pRayTracingDescriptorSet1 = FDescriptorSet::Create(GetDevice(), GetDescriptorPool(), m_pRayTracingDescriptorSetLayout);
    assert(m_pRayTracingDescriptorSet1 != nullptr);
    m_pRayTracingDescriptorSet1->SetDebugName("RayTracingPass DescriptorSet1");

    m_pRayTracingDescriptorSet1->BindAccelerationStructure(m_pScene->m_pTopLevelAS->GetAccelerationStructure(), 0);
    m_pRayTracingDescriptorSet1->BindStorageImage(m_pSceneTextureView1->GetImageView(), 1);
    m_pRayTracingDescriptorSet1->BindStorageImage(m_pSceneTextureView0->GetImageView(), 2);
    m_pRayTracingDescriptorSet1->BindUniformBuffer(m_pCameraBuffer->GetBuffer(), 3);
    m_pRayTracingDescriptorSet1->BindUniformBuffer(m_pRandomBuffer->GetBuffer(), 4);
    m_pRayTracingDescriptorSet1->BindUniformBuffer(m_pSceneSettingsBuffer->GetBuffer(), 5);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pMeshBuffer->GetBuffer(), 6);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pMaterialBuffer->GetBuffer(), 7);
}

void FRayTracer::ReleaseDescriptorSets()
{
    FBaseRenderer::ReleaseDescriptorSets();

    SAFE_DELETE(m_pRayTracingDescriptorSet0);
    SAFE_DELETE(m_pRayTracingDescriptorSet1);
}

void FRayTracer::Render(FCommandBuffer* pCommandBuffer)
{
    // Update necessary buffers
    UpdateGlobalBuffers(pCommandBuffer);

    // Update Scene
    FSceneBuffer SceneBuffer = {};
    SceneBuffer.NumMaterials          = m_pScene->m_GpuMaterials.size();
    SceneBuffer.BackgroundType        = static_cast<uint32_t>(m_pScene->m_Settings.BackgroundType);
    SceneBuffer.NumBounces            = m_pScene->m_Settings.NumBounces;
    SceneBuffer.ViewMode              = static_cast<uint32_t>(m_pScene->m_Settings.ViewMode);
    SceneBuffer.GradientLightStrength = m_pScene->m_Settings.GradientLightStrength;
    
    pCommandBuffer->UpdateBuffer(m_pSceneSettingsBuffer, 0, sizeof(FSceneBuffer), &SceneBuffer);

    // Barrier before reading the buffer from the shader
    VkMemoryBarrier MemoryBarrier;
    MemoryBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    MemoryBarrier.pNext         = nullptr;
    MemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    MemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    pCommandBuffer->PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 1, &MemoryBarrier, 0, nullptr, 0, nullptr);

    // Perform RayTracing
    pCommandBuffer->BindRayTracingPipelineState(m_pRayTracingPipeline);

    const uint64_t Frame = GetFrameIndex() % 2;
    if (Frame == 0)
    {
        pCommandBuffer->BindRayTracingDescriptorSet(m_pRayTracingPipelineLayout, m_pRayTracingDescriptorSet0, 0);
    }
    else
    {
        pCommandBuffer->BindRayTracingDescriptorSet(m_pRayTracingPipelineLayout, m_pRayTracingDescriptorSet1, 0);
    }

    pCommandBuffer->BindBindlessDescriptors(m_pRayTracingPipelineLayout, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR);

    pCommandBuffer->TraceRays(m_pRayTracingPipeline, m_pSceneTexture0->GetWidth(), m_pSceneTexture0->GetHeight(), 1);

    // Scene textures are assumed to be in GENERAL when FBaseRenderer::Render is called
    pCommandBuffer->TransitionImage(m_pSceneTexture0->GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    pCommandBuffer->TransitionImage(m_pSceneTexture1->GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

    // Tonemapping
    PerformTonemapping(pCommandBuffer);

    // Scene textures are assumed to be in GENERAL when FBaseRenderer::Render is called so let's put it back into the correct format
    pCommandBuffer->TransitionImage(m_pSceneTexture0->GetImage(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
    pCommandBuffer->TransitionImage(m_pSceneTexture1->GetImage(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
}

void FRayTracer::RenderUI()
{
    if (ImGui::Begin("Scene Inspector"))
    {
        // Select the view-mode
        {
            static const char* ViewModes[] =
            {
                "Render",
                "Normals",
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
                FDevice* pDevice = FApplication::Get().GetDevice();
                pDevice->WaitForIdle();

                // Release Descriptors
                ReleaseDescriptorSets();

                const EViewMode ViewMode = m_pScene->m_Settings.ViewMode;
                if (CurrentScene == 0) // Change to Sphere-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = FSceneFactory::CreateScene(ESceneType::Spheres);
                }
                else if (CurrentScene == 1) // Change to CornellBox-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = FSceneFactory::CreateScene(ESceneType::CornellBox);
                }
                else if (CurrentScene == 2) // Change to Triangles-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = FSceneFactory::CreateScene(ESceneType::Triangles);
                }
                else if (CurrentScene == 3) // Change to Sponza-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = FSceneFactory::CreateScene(ESceneType::Sponza);
                }
                else if (CurrentScene == 4) // Change to "Polished Glass Sphere"-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = FSceneFactory::CreateScene(ESceneType::PolishedGlassSpheres);
                }
                else if (CurrentScene == 5) // Change to "Rough Colored Glass Spheres"-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = FSceneFactory::CreateScene(ESceneType::RoughColoredGlassSpheres);
                }
                else if (CurrentScene == 6) // Change to "Rough Transparent Glass Spheres"-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = FSceneFactory::CreateScene(ESceneType::RoughTransparentGlassSpheres);
                }

                assert(m_pScene != nullptr);
                m_pScene->Initialize();
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

void FRayTracer::ReloadShaders()
{
}

void FRayTracer::CreateGlobalBuffers()
{
    FBaseRenderer::CreateGlobalBuffers();

    // SceneBuffer
    FBufferParams SceneBufferParams;
    SceneBufferParams.Size             = sizeof(FSceneBuffer);
    SceneBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    SceneBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pSceneSettingsBuffer = FBuffer::Create(GetDevice(), SceneBufferParams, nullptr);
    assert(m_pSceneSettingsBuffer != nullptr);
    m_pSceneSettingsBuffer->SetDebugName("SceneBuffer");

    // MeshBuffer
    FBufferParams MeshBufferParams = {};
    MeshBufferParams.Size             = MAX_NUM_MESHES * sizeof(FMeshInfo);
    MeshBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    MeshBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;

    m_pMeshBuffer = FBuffer::Create(GetDevice(), MeshBufferParams, nullptr);
    assert(m_pMeshBuffer != nullptr);
    m_pMeshBuffer->SetDebugName("MeshInfoBuffer");

    // MaterialBuffer
    if (!m_pScene->m_GpuMaterials.empty())
    {
        FBufferParams MaterialBufferParams;
        MaterialBufferParams.Size             = MAX_MATERIALS * sizeof(FMaterialGLSL);
        MaterialBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
        MaterialBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        m_pMaterialBuffer = FBuffer::Create(GetDevice(), MaterialBufferParams, nullptr);
        assert(m_pMaterialBuffer != nullptr);
        m_pMaterialBuffer->SetDebugName("MaterialBuffer");
    }
}

void FRayTracer::UpdateGlobalBuffers(FCommandBuffer* pCommandBuffer)
{
    // Update GPU buffers
    if (!m_pScene->m_MeshInfoBuffer.empty())
    {
        pCommandBuffer->FillBuffer(m_pMeshBuffer, 0, m_pMeshBuffer->GetSize(), 0);
        assert((sizeof(FMeshInfo) * m_pScene->m_MeshInfoBuffer.size()) <= m_pMeshBuffer->GetSize());
        pCommandBuffer->UpdateBuffer(m_pMeshBuffer, 0, sizeof(FMeshInfo) * m_pScene->m_MeshInfoBuffer.size(), m_pScene->m_MeshInfoBuffer.data());
    }

    if (!m_pScene->m_GpuMaterials.empty())
    {
        pCommandBuffer->FillBuffer(m_pMaterialBuffer, 0, m_pMaterialBuffer->GetSize(), 0);
        assert((sizeof(FMaterialGLSL) * m_pScene->m_GpuMaterials.size()) <= m_pMaterialBuffer->GetSize());
        pCommandBuffer->UpdateBuffer(m_pMaterialBuffer, 0, sizeof(FMaterialGLSL) * m_pScene->m_GpuMaterials.size(), m_pScene->m_GpuMaterials.data());
    }

    // Barrier before reading the buffer from the shader
    VkMemoryBarrier MemoryBarrier;
    MemoryBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    MemoryBarrier.pNext         = nullptr;
    MemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    MemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    pCommandBuffer->PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 1, &MemoryBarrier, 0, nullptr, 0, nullptr);
}