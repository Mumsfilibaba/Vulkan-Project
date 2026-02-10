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

CSoftwareRayTracer::CSoftwareRayTracer()
    : CBaseRenderer()
    , m_pScene(nullptr)
    , m_pSceneSettingsBuffer(nullptr)
    , m_pMaterialBuffer(nullptr)
    , m_pSphereBuffer(nullptr)
    , m_pQuadBuffer(nullptr)
    , m_pTriangleBuffer(nullptr)
    , m_pMeshBuffer(nullptr)
    , m_pVertexPositionsBuffer(nullptr)
    , m_pVertexBuffer(nullptr)
    , m_pBvhBuffer(nullptr)
    , m_pAABBVertexBuffer(nullptr)
    , m_pAABBIndexBuffer(nullptr)
    , m_pAABBInstanceBuffer(nullptr)
    , m_AABBIndexCount(0)
    , m_pRayTracingPipeline(nullptr)
    , m_pRayTracingPipelineLayout(nullptr)
    , m_pRayTracingDescriptorSetLayout(nullptr)
    , m_pRayTracingDescriptorSet0(nullptr)
    , m_pRayTracingDescriptorSet1(nullptr)
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
    , m_pDepthBufferTexture(nullptr)
    , m_pDepthBufferTextureView(nullptr)
{
}

CSoftwareRayTracer::~CSoftwareRayTracer()
{
}

void CSoftwareRayTracer::CreateResources()
{
    // Create scene
    m_pScene = new SSphereScene(ESphereSceneType::Default);
    m_pScene->Initialize();

    // RenderPasses
    CreateRayTracingResources();
    CreateDebugViewResources();
}

void CSoftwareRayTracer::ReleaseResources()
{
    SAFE_DELETE(m_pScene);

    SAFE_DELETE(m_pSceneSettingsBuffer);
    SAFE_DELETE(m_pMaterialBuffer);
    SAFE_DELETE(m_pSphereBuffer);
    SAFE_DELETE(m_pQuadBuffer);
    SAFE_DELETE(m_pTriangleBuffer);
    SAFE_DELETE(m_pMeshBuffer);
    SAFE_DELETE(m_pVertexPositionsBuffer);
    SAFE_DELETE(m_pVertexBuffer);
    SAFE_DELETE(m_pIndexBuffer);
    SAFE_DELETE(m_pBvhBuffer);
    SAFE_DELETE(m_pAABBVertexBuffer);
    SAFE_DELETE(m_pAABBIndexBuffer);
    SAFE_DELETE(m_pAABBInstanceBuffer);
    
    SAFE_DELETE(m_pRayTracingPipeline);
    SAFE_DELETE(m_pRayTracingPipelineLayout);
    SAFE_DELETE(m_pRayTracingDescriptorSetLayout);
    SAFE_DELETE(m_pRayTracingDescriptorSet0);
    SAFE_DELETE(m_pRayTracingDescriptorSet1);
    
    SAFE_DELETE(m_pDebugPipeline);
    SAFE_DELETE(m_pDebugPipelineWireframe);
    SAFE_DELETE(m_pDebugAABBPipeline);
    SAFE_DELETE(m_pDebugRenderPass);
    SAFE_DELETE(m_pDebugPipelineLayout);
    SAFE_DELETE(m_pDebugAABBPipelineLayout);
    SAFE_DELETE(m_pDebugDescriptorSetLayout);
    SAFE_DELETE(m_pDebugDescriptorSet0);
    SAFE_DELETE(m_pDebugDescriptorSet1);
    SAFE_DELETE(m_pDebugFramebuffer);

    SAFE_DELETE(m_pDepthBufferTexture);
    SAFE_DELETE(m_pDepthBufferTextureView);
}

void CSoftwareRayTracer::Render(CCommandBuffer* pCommandBuffer)
{
    // Update global buffers
    UpdateGlobalBuffers(pCommandBuffer);

    if (m_pScene->m_Settings.ViewMode != ESoftwareViewMode::Debug)
    {
        // Perform RayTracing
        PerformRayTracing(pCommandBuffer);

        // Scene textures are assumed to be in GENERAL when CBaseRenderer::Render is called
        pCommandBuffer->TransitionImage(m_pSceneTexture0->GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        pCommandBuffer->TransitionImage(m_pSceneTexture1->GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

        // Tonemapping
        PerformTonemapping(pCommandBuffer);

        // Scene textures are assumed to be in GENERAL when CBaseRenderer::Render is called so let's put it back into the correct format
        pCommandBuffer->TransitionImage(m_pSceneTexture0->GetImage(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
        pCommandBuffer->TransitionImage(m_pSceneTexture1->GetImage(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    else
    {
        // Rasterize triangle models and display the BVH
        PerformDebugPass(pCommandBuffer);
    }
}

void CSoftwareRayTracer::PerformRayTracing(CCommandBuffer* pCommandBuffer)
{
    // Update Scene
    SSoftwareSceneBuffer SceneBuffer = {};
    SceneBuffer.NumQuads              = m_pScene->m_Quads.size();
    SceneBuffer.NumSpheres            = m_pScene->m_Spheres.size();
    SceneBuffer.NumMaterials          = m_pScene->m_GpuMaterials.size();
    SceneBuffer.NumMeshes             = m_pScene->m_Meshes.size();
    SceneBuffer.NumBvhNodes           = m_pScene->m_AccelerationStructure.m_BoundingBoxes.size();
    SceneBuffer.NumTriangles          = m_pScene->m_AccelerationStructure.m_TriangleInfo.size();
    SceneBuffer.BackgroundType        = m_pScene->m_Settings.BackgroundType;
    SceneBuffer.NumBounces            = m_pScene->m_Settings.NumBounces;
    SceneBuffer.ViewMode              = static_cast<uint32_t>(m_pScene->m_Settings.ViewMode);
    SceneBuffer.GradientLightStrength = m_pScene->m_Settings.GradientLightStrength;
    
    pCommandBuffer->UpdateBuffer(m_pSceneSettingsBuffer, 0, sizeof(SSoftwareSceneBuffer), &SceneBuffer);

    // Barrier before reading the buffer from the shader
    VkMemoryBarrier MemoryBarrier;
    MemoryBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    MemoryBarrier.pNext         = nullptr;
    MemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    MemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    pCommandBuffer->PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &MemoryBarrier, 0, nullptr, 0, nullptr);

    // Bind pipeline and descriptorSet
    pCommandBuffer->BindComputePipelineState(m_pRayTracingPipeline.load());

    const uint64_t Frame = GetFrameIndex() % 2;
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
}

void CSoftwareRayTracer::PerformDebugPass(CCommandBuffer* pCommandBuffer)
{
    // Define clear colors
    VkClearValue ClearColor[2];
    ClearColor[0].color        = { 0.0f, 0.0f, 0.0f, 1.0f };
    ClearColor[1].depthStencil = { 1.0f, 0 };

    if (!m_pScene->m_pVertexPositionsBuffer || !m_pScene->m_pIndexBuffer)
    {
        // Begin RenderPass (Only clear the image when the buffers are invalid)
        pCommandBuffer->BeginRenderPass(m_pDebugRenderPass, m_pDebugFramebuffer, ClearColor, 2);

        // End RenderPass
        pCommandBuffer->EndRenderPass();
        return;
    }

    // Begin RenderPass
    pCommandBuffer->BeginRenderPass(m_pDebugRenderPass, m_pDebugFramebuffer, ClearColor, 2);

    // Set viewport
    VkViewport Viewport;
    Viewport.width    =  static_cast<float>(GetViewportWidth());
    Viewport.height   = -static_cast<float>(GetViewportHeight());
    Viewport.minDepth =  0.0f;
    Viewport.maxDepth =  1.0f;
    Viewport.x        =  0.0f;
    Viewport.y        =  static_cast<float>(GetViewportHeight());

    pCommandBuffer->SetViewport(Viewport);

    VkRect2D scissor = { { 0, 0}, { GetViewportWidth(), GetViewportHeight() } };
    pCommandBuffer->SetScissorRect(scissor);
    
    // Draw Mesh
    {
        pCommandBuffer->BindGraphicsPipelineState(m_pDebugPipeline);

        const glm::vec4 Color = glm::vec4(0.9f, 0.9f, 0.9f, 1.0f);
        pCommandBuffer->PushConstants(m_pDebugPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(glm::vec4), glm::value_ptr(Color));

        // Bind DescriptorSets
        const uint64_t Frame = GetFrameIndex() % 2;
        if (Frame == 0)
        {
            pCommandBuffer->BindGraphicsDescriptorSet(m_pDebugPipelineLayout, m_pDebugDescriptorSet0, 0);
        }
        else
        {
            pCommandBuffer->BindGraphicsDescriptorSet(m_pDebugPipelineLayout, m_pDebugDescriptorSet1, 0);
        }

        // Set Vertex- and IndexBuffer
        pCommandBuffer->BindVertexBuffer(m_pScene->m_pVertexPositionsBuffer, 0, 0);
        pCommandBuffer->BindIndexBuffer(m_pScene->m_pIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

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
        const uint64_t Frame = GetFrameIndex() % 2;
        if (Frame == 0)
        {
            pCommandBuffer->BindGraphicsDescriptorSet(m_pDebugPipelineLayout, m_pDebugDescriptorSet0, 0);
        }
        else
        {
            pCommandBuffer->BindGraphicsDescriptorSet(m_pDebugPipelineLayout, m_pDebugDescriptorSet1, 0);
        }

        // Set Vertex- and IndexBuffer
        pCommandBuffer->BindVertexBuffer(m_pScene->m_pVertexPositionsBuffer, 0, 0);
        pCommandBuffer->BindIndexBuffer(m_pScene->m_pIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

        // Draw
        const size_t IndexCount = m_pScene->m_Indicies.size();
        pCommandBuffer->DrawIndexInstanced(IndexCount, 1, 0, 0, 0);
    }

    // Draw the bounding boxes
    {
        pCommandBuffer->BindGraphicsPipelineState(m_pDebugAABBPipeline);

        // Bind DescriptorSets
        const uint64_t Frame = GetFrameIndex() % 2;
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

        struct SAABBDebugData
        {
            glm::vec4 Color;
        } DebugData;

        DebugData.Color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        pCommandBuffer->PushConstants(m_pDebugAABBPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(SAABBDebugData), &DebugData);

        const uint32_t NumInstances = static_cast<uint32_t>(m_pScene->m_AccelerationStructure.m_BoundingBoxes.size());
        pCommandBuffer->DrawIndexInstanced(m_AABBIndexCount, NumInstances, 0, 0, 0);
    }

    // End RenderPass
    pCommandBuffer->EndRenderPass();
}

void CSoftwareRayTracer::RenderUI()
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
                "BVH Intersection",
                "Debug"
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
                    m_pScene->m_Settings.ViewMode = ESoftwareViewMode::Render;
                }
                else if (CurrentViewMode == 1)
                {
                    m_pScene->m_Settings.ViewMode = ESoftwareViewMode::Normals;
                }
                else if (CurrentViewMode == 2)
                {
                    m_pScene->m_Settings.ViewMode = ESoftwareViewMode::Albedo;
                }
                else if (CurrentViewMode == 3)
                {
                    m_pScene->m_Settings.ViewMode = ESoftwareViewMode::Barycentrics;
                }
                else if (CurrentViewMode == 4)
                {
                    m_pScene->m_Settings.ViewMode = ESoftwareViewMode::TexCoords;
                }
                else if (CurrentViewMode == 5)
                {
                    m_pScene->m_Settings.ViewMode = ESoftwareViewMode::BVHIntersection;
                }
                else if (CurrentViewMode == 6)
                {
                    m_pScene->m_Settings.ViewMode = ESoftwareViewMode::Debug;
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
                const ESoftwareViewMode ViewMode = m_pScene->m_Settings.ViewMode;
                if (CurrentScene == 0) // Change to Sphere-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = new SSphereScene(ESphereSceneType::Default);
                }
                else if (CurrentScene == 1) // Change to CornellBox-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = new SCornellBoxScene();
                }
                else if (CurrentScene == 2) // Change to Triangles-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = new SModelScene(EModelSceneType::Default);
                }
                else if (CurrentScene == 3) // Change to "Polished Glass Sphere"-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = new SModelScene(EModelSceneType::Sponza);
                }
                else if (CurrentScene == 4) // Change to "Polished Glass Sphere"-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = new SSphereScene(ESphereSceneType::PolishedGlass);
                }
                else if (CurrentScene == 5) // Change to "Rough Colored Glass Spheres"-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = new SSphereScene(ESphereSceneType::ColoredRoughGlass);
                }
                else if (CurrentScene == 6) // Change to "Rough Transparent Glass Spheres"-scene
                {
                    SAFE_DELETE(m_pScene);
                    m_pScene = new SSphereScene(ESphereSceneType::RoughGlass);
                }

                assert(m_pScene != nullptr);
                m_pScene->Initialize();
                ResetImage();

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
        }

        // Reset the scene
        if (ImGui::Button("Reset Camera"))
        {
            m_pScene->Reset();
            ResetImage();
        }

        ImGui::NewLine();

        ImGui::Text("Objects:");
        ImGui::Separator();

        uint32_t ImguiID = 0;
        {
            uint32_t Index = 1;
            for (SSphereGLSL& Sphere : m_pScene->m_Spheres)
            {
                ImGui::PushID(ImguiID++);

                ImGui::Text("Sphere %d", Index++);
                if (ImGui::DragFloat3("Position", glm::value_ptr(Sphere.Position), 0.1f))
                {
                    ResetImage();
                }
                if (ImGui::DragFloat("Radius", &Sphere.Radius, 0.01f))
                {
                    ResetImage();
                }

                ImGui::PopID();
                ImGui::Separator();
            }
        }

        {
            uint32_t Index = 1;
            for (SQuadGLSL& Quad : m_pScene->m_Quads)
            {
                ImGui::PushID(ImguiID++);

                ImGui::Text("Quad %d", Index++);
                if (ImGui::DragFloat3("Position", glm::value_ptr(Quad.Position), 0.1f))
                {
                    ResetImage();
                }
                if (ImGui::DragFloat3("Edge0", glm::value_ptr(Quad.Edge0), 0.1f))
                {
                    ResetImage();
                }
                if (ImGui::DragFloat3("Edge1", glm::value_ptr(Quad.Edge1), 0.1f))
                {
                    ResetImage();
                }

                ImGui::PopID();
                ImGui::Separator();
            }
        }

        {
            uint32_t Index = 1;
            for (SMeshGLSL& Mesh : m_pScene->m_Meshes)
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
            for (SMaterialGLSL& Material : m_pScene->m_GpuMaterials)
            {
                ImGui::PushID(ImguiID++);
                ImGui::Text("Material %d", Index++);

                // if (ImGui::ColorEdit3("Albedo", glm::value_ptr(material.Albedo)))
                if (ImGui::InputFloat3("AlbedoColor", glm::value_ptr(Material.AlbedoColor)))
                {
                    ResetImage();
                }
                // if (ImGui::ColorEdit3("Emissive", glm::value_ptr(material.Emissive)))
                if (ImGui::InputFloat3("EmissiveColor", glm::value_ptr(Material.EmissiveColor)))
                {
                    ResetImage();
                }
                // if (ImGui::ColorEdit3("Emissive", glm::value_ptr(material.Emissive)))
                if (ImGui::InputFloat3("SpecularColor", glm::value_ptr(Material.SpecularColor)))
                {
                    ResetImage();
                }
                // if (ImGui::ColorEdit3("Emissive", glm::value_ptr(material.AbsorbtionColor)))
                if (ImGui::InputFloat3("AbsorbtionColor", glm::value_ptr(Material.AbsorbtionColor)))
                {
                    ResetImage();
                }
                if (ImGui::DragFloat("SpecularChance", &Material.SpecularChance, 0.1f, 0.0f, 1.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
                {
                    ResetImage();
                }
                if (ImGui::DragFloat("SpecularRoughness", &Material.SpecularRoughness, 0.1f, 0.0f, 1.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
                {
                    ResetImage();
                }
                if (ImGui::DragFloat("RefractionChance", &Material.RefractionChance, 0.1f, 0.0f, 1.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
                {
                    ResetImage();
                }
                if (ImGui::DragFloat("RefractionRoughness", &Material.RefractionRoughness, 0.1f, 0.0f, 1.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
                {
                    ResetImage();
                }
                if (ImGui::DragFloat("IncidenceOfRefraction", &Material.IncidenceOfRefraction, 0.1f, 0.5f, 2.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp))
                {
                    ResetImage();
                }

                ImGui::PopID();
                ImGui::Separator();
            }
        }

        ImGui::End();
    }
}

void CSoftwareRayTracer::CreateRayTracingResources()
{
    // Create RayTracing DescriptorSetLayout
    constexpr uint32_t NumRayTracingBindings = 15;
    VkDescriptorSetLayoutBinding RayTracingBindings[NumRayTracingBindings];

    // OutputImage
    RayTracingBindings[0].binding            = 0;
    RayTracingBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    RayTracingBindings[0].descriptorCount    = 1;
    RayTracingBindings[0].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[0].pImmutableSamplers = nullptr;

    // AccumulationImage
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

    // CameraBuffer
    RayTracingBindings[3].binding            = 3;
    RayTracingBindings[3].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    RayTracingBindings[3].descriptorCount    = 1;
    RayTracingBindings[3].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[3].pImmutableSamplers = nullptr;

    // RandomBuffer
    RayTracingBindings[4].binding            = 4;
    RayTracingBindings[4].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    RayTracingBindings[4].descriptorCount    = 1;
    RayTracingBindings[4].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[4].pImmutableSamplers = nullptr;

    // SceneBuffer
    RayTracingBindings[5].binding            = 5;
    RayTracingBindings[5].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    RayTracingBindings[5].descriptorCount    = 1;
    RayTracingBindings[5].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[5].pImmutableSamplers = nullptr;

    // QuadsBuffer
    RayTracingBindings[6].binding            = 6;
    RayTracingBindings[6].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[6].descriptorCount    = 1;
    RayTracingBindings[6].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[6].pImmutableSamplers = nullptr;

    // SpheresBuffer
    RayTracingBindings[7].binding            = 7;
    RayTracingBindings[7].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[7].descriptorCount    = 1;
    RayTracingBindings[7].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[7].pImmutableSamplers = nullptr;

    // MaterialBuffer
    RayTracingBindings[8].binding            = 8;
    RayTracingBindings[8].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[8].descriptorCount    = 1;
    RayTracingBindings[8].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[8].pImmutableSamplers = nullptr;

    // VertexPositionsBuffer
    RayTracingBindings[9].binding            = 9;
    RayTracingBindings[9].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[9].descriptorCount    = 1;
    RayTracingBindings[9].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[9].pImmutableSamplers = nullptr;

    // VertexBuffer
    RayTracingBindings[10].binding            = 10;
    RayTracingBindings[10].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[10].descriptorCount    = 1;
    RayTracingBindings[10].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[10].pImmutableSamplers = nullptr;

    // IndexBuffer
    RayTracingBindings[11].binding            = 11;
    RayTracingBindings[11].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[11].descriptorCount    = 1;
    RayTracingBindings[11].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[11].pImmutableSamplers = nullptr;

    // TriangleBuffer
    RayTracingBindings[12].binding            = 12;
    RayTracingBindings[12].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[12].descriptorCount    = 1;
    RayTracingBindings[12].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[12].pImmutableSamplers = nullptr;

    // MeshBuffer
    RayTracingBindings[13].binding            = 13;
    RayTracingBindings[13].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[13].descriptorCount    = 1;
    RayTracingBindings[13].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[13].pImmutableSamplers = nullptr;

    // BVH-Buffer
    RayTracingBindings[14].binding            = 14;
    RayTracingBindings[14].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[14].descriptorCount    = 1;
    RayTracingBindings[14].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    RayTracingBindings[14].pImmutableSamplers = nullptr;

    SDescriptorSetLayoutParams RayTracingDescriptorSetLayoutParams;
    RayTracingDescriptorSetLayoutParams.pBindings   = RayTracingBindings;
    RayTracingDescriptorSetLayoutParams.NumBindings = NumRayTracingBindings;

    m_pRayTracingDescriptorSetLayout = CDescriptorSetLayout::Create(GetDevice(), RayTracingDescriptorSetLayoutParams);
    assert(m_pRayTracingDescriptorSetLayout != nullptr);
    m_pRayTracingDescriptorSetLayout->SetDebugName("RayTracingPass DescriptorSetLayout");

    // Create RayTracing PipelineLayout
    SPipelineLayoutParams RayTracingPipelineLayoutParams;
    RayTracingPipelineLayoutParams.ppLayouts       = &m_pRayTracingDescriptorSetLayout;
    RayTracingPipelineLayoutParams.NumLayouts      = 1;
    RayTracingPipelineLayoutParams.bEnableBindless = true;

    m_pRayTracingPipelineLayout = CPipelineLayout::Create(GetDevice(), RayTracingPipelineLayoutParams);
    assert(m_pRayTracingPipelineLayout != nullptr);
    m_pRayTracingPipelineLayout->SetDebugName("RayTracingPass PipelineLayout");

    // Create RayTracing shader and pipeline
    CShaderModule* pComputeShader = CShaderModule::CreateFromFile(GetDevice(), "main", RESOURCE_PATH"/shaders/raytracer.spv");
    assert(pComputeShader != nullptr);
    pComputeShader->SetDebugName(RESOURCE_PATH"/shaders/raytracer.spv");

    SComputePipelineStateParams PipelineParams = {};
    PipelineParams.pShader         = pComputeShader;
    PipelineParams.pPipelineLayout = m_pRayTracingPipelineLayout;

    m_pRayTracingPipeline = CComputePipeline::Create(GetDevice(), PipelineParams);
    assert(m_pRayTracingPipeline != nullptr);
    m_pRayTracingPipeline.load()->SetDebugName("RayTracingPass Pipeline");

    SAFE_DELETE(pComputeShader);
}

void CSoftwareRayTracer::CreateDebugViewResources()
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

    SDescriptorSetLayoutParams DebugPassDescriptorSetLayoutParams;
    DebugPassDescriptorSetLayoutParams.pBindings   = DebugPassBindings;
    DebugPassDescriptorSetLayoutParams.NumBindings = NumDebugPassBindings;

    m_pDebugDescriptorSetLayout = CDescriptorSetLayout::Create(GetDevice(), DebugPassDescriptorSetLayoutParams);
    assert(m_pDebugDescriptorSetLayout != nullptr);
    m_pDebugDescriptorSetLayout->SetDebugName("DebugPass DescriptorSetLayout");

    // Create DebugPass PipelineLayout
    SPipelineLayoutParams DebugPassPipelineLayoutParams;
    DebugPassPipelineLayoutParams.ppLayouts        = &m_pDebugDescriptorSetLayout;
    DebugPassPipelineLayoutParams.NumLayouts       = 1;
    DebugPassPipelineLayoutParams.NumPushConstants = 4;

    m_pDebugPipelineLayout = CPipelineLayout::Create(GetDevice(), DebugPassPipelineLayoutParams);
    assert(m_pDebugPipelineLayout != nullptr);
    m_pDebugPipelineLayout->SetDebugName("DebugPass PipelineLayout");

    DebugPassPipelineLayoutParams.NumPushConstants = 20;

    m_pDebugAABBPipelineLayout = CPipelineLayout::Create(GetDevice(), DebugPassPipelineLayoutParams);
    assert(m_pDebugAABBPipelineLayout != nullptr);
    m_pDebugAABBPipelineLayout->SetDebugName("DebugPass AABB PipelineLayout");

    // PipelineState, RenderPass and Shaders
    CShaderModule* pVertex = CShaderModule::CreateFromFile(GetDevice(), "main", RESOURCE_PATH"/shaders/vertex.spv");
    assert(pVertex != nullptr);
    pVertex->SetDebugName(RESOURCE_PATH"/shaders/vertex.spv");

    CShaderModule* pAABBVertex = CShaderModule::CreateFromFile(GetDevice(), "main", RESOURCE_PATH"/shaders/aabb_debug_vs.spv");
    assert(pAABBVertex != nullptr);
    pAABBVertex->SetDebugName(RESOURCE_PATH"/shaders/aabb_debug_vs.spv");

    CShaderModule* pFragment = CShaderModule::CreateFromFile(GetDevice(), "main", RESOURCE_PATH"/shaders/fragment.spv");
    assert(pFragment != nullptr);
    pFragment->SetDebugName(RESOURCE_PATH"/shaders/fragment.spv");

    CShaderModule* pAABBFragment = CShaderModule::CreateFromFile(GetDevice(), "main", RESOURCE_PATH"/shaders/aabb_debug_fs.spv");
    assert(pAABBFragment != nullptr);
    pAABBFragment->SetDebugName(RESOURCE_PATH"/shaders/aabb_debug_fs.spv");

    SRenderPassAttachment ColorAttachments[1];
    ColorAttachments[0].Format        = VK_FORMAT_R8G8B8A8_UNORM;
    ColorAttachments[0].InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ColorAttachments[0].FinalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    SRenderPassAttachment DepthAttachment[1];
    DepthAttachment[0].Format        = VK_FORMAT_D24_UNORM_S8_UINT;
    DepthAttachment[0].InitialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    DepthAttachment[0].FinalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    SRenderPassParams RenderPassParams = {};
    RenderPassParams.pColorAttachments    = ColorAttachments;
    RenderPassParams.ColorAttachmentCount = 1;
    RenderPassParams.pDepthAttachment     = DepthAttachment;

    m_pDebugRenderPass = CRenderPass::Create(GetDevice(), RenderPassParams);
    assert(m_pDebugRenderPass != nullptr);
    m_pDebugRenderPass->SetDebugName("DebugPass RenderPass");

    SGraphicsPipelineStateParams DebugPassPipelineParams = {};
    DebugPassPipelineParams.pBindingDescriptions      = SVertexPosition::GetBindingDescription();
    DebugPassPipelineParams.BindingDescriptionCount   = 1;
    DebugPassPipelineParams.pAttributeDescriptions    = SVertexPosition::GetAttributeDescriptions();
    DebugPassPipelineParams.AttributeDescriptionCount = 1;
    DebugPassPipelineParams.pVertexShader             = pVertex;
    DebugPassPipelineParams.pFragmentShader           = pFragment;
    DebugPassPipelineParams.pRenderPass               = m_pDebugRenderPass;
    DebugPassPipelineParams.pPipelineLayout           = m_pDebugPipelineLayout;
    DebugPassPipelineParams.bDepthEnable              = true;

    m_pDebugPipeline = CGraphicsPipeline::Create(GetDevice(), DebugPassPipelineParams);
    assert(m_pDebugPipeline != nullptr);
    m_pDebugPipeline->SetDebugName("DebugPass Pipeline");

    DebugPassPipelineParams.PolygonMode = VK_POLYGON_MODE_LINE;

    m_pDebugPipelineWireframe = CGraphicsPipeline::Create(GetDevice(), DebugPassPipelineParams);
    assert(m_pDebugPipelineWireframe != nullptr);
    m_pDebugPipelineWireframe->SetDebugName("DebugPass Pipeline WireFrame");

    DebugPassPipelineParams.pBindingDescriptions      = SVertexPosition::GetBindingDescription();
    DebugPassPipelineParams.BindingDescriptionCount   = 1;
    DebugPassPipelineParams.pAttributeDescriptions    = SVertexPosition::GetAttributeDescriptions();
    DebugPassPipelineParams.AttributeDescriptionCount = 1;
    DebugPassPipelineParams.pVertexShader             = pAABBVertex;
    DebugPassPipelineParams.pFragmentShader           = pAABBFragment;
    DebugPassPipelineParams.pRenderPass               = m_pDebugRenderPass;
    DebugPassPipelineParams.pPipelineLayout           = m_pDebugAABBPipelineLayout;
    DebugPassPipelineParams.Topology                  = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    DebugPassPipelineParams.bDepthEnable              = true;

    m_pDebugAABBPipeline = CGraphicsPipeline::Create(GetDevice(), DebugPassPipelineParams);
    assert(m_pDebugAABBPipeline != nullptr);
    m_pDebugAABBPipeline->SetDebugName("DebugPass AABB Pipeline");

    SAFE_DELETE(pVertex);
    SAFE_DELETE(pAABBVertex);
    SAFE_DELETE(pFragment);
    SAFE_DELETE(pAABBFragment);

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
    SBufferParams BufferParams;
    BufferParams.Size             = sizeof(glm::vec3) * AABBVertices.size();
    BufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;
    BufferParams.Usage            = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pAABBVertexBuffer = CBuffer::CreateWithData(GetDevice(), BufferParams, nullptr, AABBVertices.data());
    assert(m_pAABBVertexBuffer != nullptr);
    m_pAABBVertexBuffer->SetDebugName("AABBVertexBuffer");

    BufferParams.Size  = sizeof(uint32_t) * AABBIndices.size();
    BufferParams.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    m_AABBIndexCount = AABBIndices.size();

    m_pAABBIndexBuffer = CBuffer::CreateWithData(GetDevice(), BufferParams, nullptr, AABBIndices.data());
    assert(m_pAABBIndexBuffer != nullptr);
    m_pAABBIndexBuffer->SetDebugName("AABBIndexBuffer");
}

void CSoftwareRayTracer::CreateGlobalBuffers()
{
    CBaseRenderer::CreateGlobalBuffers();

    // SceneBuffer
    SBufferParams SceneBufferParams;
    SceneBufferParams.Size             = sizeof(SSoftwareSceneBuffer);
    SceneBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    SceneBufferParams.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pSceneSettingsBuffer = CBuffer::Create(GetDevice(), SceneBufferParams, GetDeviceAllocator());
    assert(m_pSceneSettingsBuffer != nullptr);
    m_pSceneSettingsBuffer->SetDebugName("SceneBuffer");

    // QuadBuffer
    SBufferParams QuadBufferParams;
    QuadBufferParams.Size             = sizeof(SQuadGLSL) * MAX_QUADS;
    QuadBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    QuadBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pQuadBuffer = CBuffer::Create(GetDevice(), QuadBufferParams, GetDeviceAllocator());
    assert(m_pQuadBuffer != nullptr);
    m_pQuadBuffer->SetDebugName("QuadBuffer");

    // SphereBuffer
    SBufferParams SphereBufferParams;
    SphereBufferParams.Size             = sizeof(SSphereGLSL) * MAX_SPHERES;
    SphereBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    SphereBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pSphereBuffer = CBuffer::Create(GetDevice(), SphereBufferParams, GetDeviceAllocator());
    assert(m_pSphereBuffer != nullptr);
    m_pSphereBuffer->SetDebugName("SphereBuffer");

    // VertexBuffer
    SBufferParams VertexPositionsBufferParams;
    VertexPositionsBufferParams.Size             = sizeof(SVertexPosition) * MAX_VERTICES;
    VertexPositionsBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    VertexPositionsBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pVertexPositionsBuffer = CBuffer::Create(GetDevice(), VertexPositionsBufferParams, GetDeviceAllocator());
    assert(m_pVertexPositionsBuffer != nullptr);
    m_pVertexPositionsBuffer->SetDebugName("VertexPositionsBuffer");

    SBufferParams VertexBufferParams;
    VertexBufferParams.Size             = sizeof(SVertex) * MAX_VERTICES;
    VertexBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    VertexBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pVertexBuffer = CBuffer::Create(GetDevice(), VertexBufferParams, GetDeviceAllocator());
    assert(m_pVertexBuffer != nullptr);
    m_pVertexBuffer->SetDebugName("VertexBuffer");

    // IndexBuffer
    SBufferParams IndexBufferParams;
    IndexBufferParams.Size             = (sizeof(uint32_t) * 3) * MAX_TRIANGLES;
    IndexBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    IndexBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pIndexBuffer = CBuffer::Create(GetDevice(), IndexBufferParams, GetDeviceAllocator());
    assert(m_pIndexBuffer  != nullptr);
    m_pIndexBuffer->SetDebugName("IndexBuffer");

    // TriangleBuffer
    SBufferParams TriangleBufferParams;
    TriangleBufferParams.Size             = sizeof(STriangleInfoGLSL) * MAX_TRIANGLES;
    TriangleBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    TriangleBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pTriangleBuffer = CBuffer::Create(GetDevice(), TriangleBufferParams, GetDeviceAllocator());
    assert(m_pTriangleBuffer != nullptr);
    m_pTriangleBuffer->SetDebugName("TriangleBuffer");

    // TriangleMeshesBuffer
    SBufferParams MeshBufferParams;
    MeshBufferParams.Size             = sizeof(SMeshGLSL) * MAX_TRIANGLEMESHES;
    MeshBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    MeshBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pMeshBuffer = CBuffer::Create(GetDevice(), MeshBufferParams, GetDeviceAllocator());
    assert(m_pMeshBuffer != nullptr);
    m_pMeshBuffer->SetDebugName("MeshBuffer");

    // MaterialBuffer
    SBufferParams MaterialBufferParams;
    MaterialBufferParams.Size             = sizeof(SMaterialGLSL) * MAX_MATERIALS;
    MaterialBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    MaterialBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pMaterialBuffer = CBuffer::Create(GetDevice(), MaterialBufferParams, GetDeviceAllocator());
    assert(m_pMaterialBuffer != nullptr);
    m_pMaterialBuffer->SetDebugName("MaterialBuffer");

    // BoundingBoxBuffer
    SBufferParams BoundingBoxBufferParams;
    BoundingBoxBufferParams.Size             = sizeof(SShaderBoundingBox) * MAX_BVH_NODES;
    BoundingBoxBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    BoundingBoxBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pBvhBuffer = CBuffer::Create(GetDevice(), BoundingBoxBufferParams, GetDeviceAllocator());
    assert(m_pBvhBuffer != nullptr);
    m_pBvhBuffer->SetDebugName("BVHBuffer");

    // Debug AABB instance buffer
    SBufferParams AABBInstanceBufferParams;
    AABBInstanceBufferParams.Size             = sizeof(glm::mat4) * MAX_BVH_NODES;
    AABBInstanceBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    AABBInstanceBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pAABBInstanceBuffer = CBuffer::Create(GetDevice(), AABBInstanceBufferParams, GetDeviceAllocator());
    assert(m_pAABBInstanceBuffer != nullptr);
    m_pAABBInstanceBuffer->SetDebugName("Debug AABB Instance Buffer");
}

void CSoftwareRayTracer::CreateDescriptorSets()
{
    // Create common DescriptorSets
    CBaseRenderer::CreateDescriptorSets();

    // RayTracing Pass
    m_pRayTracingDescriptorSet0 = CDescriptorSet::Create(GetDevice(), GetDescriptorPool(), m_pRayTracingDescriptorSetLayout);
    assert(m_pRayTracingDescriptorSet0 != nullptr);
    m_pRayTracingDescriptorSet0->SetDebugName("RayTracingPass DescriptorSet0");

    m_pRayTracingDescriptorSet0->BindStorageImage(m_pSceneTextureView0->GetImageView(), 0);
    m_pRayTracingDescriptorSet0->BindStorageImage(m_pSceneTextureView1->GetImageView(), 1);
    m_pRayTracingDescriptorSet0->BindCombinedImageSampler(m_pSkybox->GetTextureView()->GetImageView(), m_pSkyboxSampler->GetSampler(), 2);
    m_pRayTracingDescriptorSet0->BindUniformBuffer(m_pCameraBuffer->GetBuffer(), 3);
    m_pRayTracingDescriptorSet0->BindUniformBuffer(m_pRandomBuffer->GetBuffer(), 4);
    m_pRayTracingDescriptorSet0->BindUniformBuffer(m_pSceneSettingsBuffer->GetBuffer(), 5);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pQuadBuffer->GetBuffer(), 6);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pSphereBuffer->GetBuffer(), 7);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pMaterialBuffer->GetBuffer(), 8);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pVertexPositionsBuffer->GetBuffer(), 9);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pVertexBuffer->GetBuffer(), 10);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pIndexBuffer->GetBuffer(), 11);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pTriangleBuffer->GetBuffer(), 12);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pMeshBuffer->GetBuffer(), 13);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pBvhBuffer->GetBuffer(), 14);
    
    m_pRayTracingDescriptorSet1 = CDescriptorSet::Create(GetDevice(), GetDescriptorPool(), m_pRayTracingDescriptorSetLayout);
    assert(m_pRayTracingDescriptorSet1 != nullptr);
    m_pRayTracingDescriptorSet1->SetDebugName("RayTracingPass DescriptorSet1");

    m_pRayTracingDescriptorSet1->BindStorageImage(m_pSceneTextureView1->GetImageView(), 0);
    m_pRayTracingDescriptorSet1->BindStorageImage(m_pSceneTextureView0->GetImageView(), 1);
    m_pRayTracingDescriptorSet1->BindCombinedImageSampler(m_pSkybox->GetTextureView()->GetImageView(), m_pSkyboxSampler->GetSampler(), 2);
    m_pRayTracingDescriptorSet1->BindUniformBuffer(m_pCameraBuffer->GetBuffer(), 3);
    m_pRayTracingDescriptorSet1->BindUniformBuffer(m_pRandomBuffer->GetBuffer(), 4);
    m_pRayTracingDescriptorSet1->BindUniformBuffer(m_pSceneSettingsBuffer->GetBuffer(), 5);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pQuadBuffer->GetBuffer(), 6);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pSphereBuffer->GetBuffer(), 7);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pMaterialBuffer->GetBuffer(), 8);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pVertexPositionsBuffer->GetBuffer(), 9);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pVertexBuffer->GetBuffer(), 10);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pIndexBuffer->GetBuffer(), 11);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pTriangleBuffer->GetBuffer(), 12);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pMeshBuffer->GetBuffer(), 13);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pBvhBuffer->GetBuffer(), 14);

    // Debug Pass
    m_pDebugDescriptorSet0 = CDescriptorSet::Create(GetDevice(), GetDescriptorPool(), m_pDebugDescriptorSetLayout);
    assert(m_pDebugDescriptorSet0 != nullptr);
    m_pDebugDescriptorSet0->SetDebugName("DebugPass DescriptorSet0");

    m_pDebugDescriptorSet0->BindUniformBuffer(m_pCameraBuffer->GetBuffer(), 0);
    m_pDebugDescriptorSet0->BindStorageBuffer(m_pAABBInstanceBuffer->GetBuffer(), 1);
    
    m_pDebugDescriptorSet1 = CDescriptorSet::Create(GetDevice(), GetDescriptorPool(), m_pDebugDescriptorSetLayout);
    assert(m_pDebugDescriptorSet1 != nullptr);
    m_pDebugDescriptorSet1->SetDebugName("DebugPass DescriptorSet1");

    m_pDebugDescriptorSet1->BindUniformBuffer(m_pCameraBuffer->GetBuffer(), 0);
    m_pDebugDescriptorSet1->BindStorageBuffer(m_pAABBInstanceBuffer->GetBuffer(), 1);
}

void CSoftwareRayTracer::ReleaseDescriptorSets()
{
    CBaseRenderer::ReleaseDescriptorSets();

    SAFE_DELETE(m_pRayTracingDescriptorSet0);
    SAFE_DELETE(m_pRayTracingDescriptorSet1);
    SAFE_DELETE(m_pDebugDescriptorSet0);
    SAFE_DELETE(m_pDebugDescriptorSet1);
}

bool CSoftwareRayTracer::CreateOrResizeSceneTexture(uint32_t Width, uint32_t Height)
{
    if (!CBaseRenderer::CreateOrResizeSceneTexture(Width, Height))
    {
        // No resize happened so we return here as well
        return false;
    }

    SAFE_DELETE(m_pDepthBufferTexture);
    SAFE_DELETE(m_pDepthBufferTextureView);
    SAFE_DELETE(m_pDebugFramebuffer);

    // Create depth-buffer texture for the viewport
    STextureParams DepthBufferParams = {};
    DepthBufferParams.Format        = VK_FORMAT_D24_UNORM_S8_UINT;
    DepthBufferParams.ImageType     = VK_IMAGE_TYPE_2D;
    DepthBufferParams.Width         = Width;
    DepthBufferParams.Height        = Height;
    DepthBufferParams.Usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    DepthBufferParams.InitialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    m_pDepthBufferTexture = CTexture::Create(GetDevice(), DepthBufferParams);
    assert(m_pDepthBufferTexture != nullptr);
    m_pDepthBufferTexture->SetDebugName("DepthBuffer");
    
    {
        STextureViewParams TextureViewParams = {};
        TextureViewParams.pTexture = m_pDepthBufferTexture;
        
        m_pDepthBufferTextureView = CTextureView::Create(GetDevice(), TextureViewParams);
        assert(m_pDepthBufferTextureView != nullptr);
        m_pDepthBufferTextureView->SetDebugName("DepthBufferView");
    }

    // Create Framebuffer for the DebugView stage
    VkImageView DebugPassImageViews[] =
    {
        m_pOutputTextureView->GetImageView(),
        m_pDepthBufferTextureView->GetImageView()
    };
    
    SFramebufferParams FramebufferParams = {};
    FramebufferParams.AttachmentCount = 2;
    FramebufferParams.Width           = Width;
    FramebufferParams.Height          = Height;
    FramebufferParams.pRenderPass     = m_pDebugRenderPass;
    FramebufferParams.pAttachMents    = DebugPassImageViews;

    m_pDebugFramebuffer = CFramebuffer::Create(GetDevice(), FramebufferParams);
    m_pDebugFramebuffer->SetDebugName("DebugPass FrameBuffer");
}

void CSoftwareRayTracer::ReloadShaders()
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
            CShaderModule* pComputeShader = CShaderModule::CreateFromFile(GetDevice(), "main", RESOURCE_PATH"/shaders/raytracer.spv");
            if (!pComputeShader)
            {
                LOG("FAILED to create ComputeShader\n");
                bIsCompiling = false;
                return false;
            }

            SComputePipelineStateParams pipelineParams = {};
            pipelineParams.pShader         = pComputeShader;
            pipelineParams.pPipelineLayout = m_pRayTracingPipelineLayout;

            CComputePipeline* pComputePipeline  = CComputePipeline::Create(GetDevice(), pipelineParams);
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

            GetDevice()->WaitForIdle();
            pComputePipeline = m_pRayTracingPipeline.exchange(pComputePipeline);

            SAFE_DELETE(pComputeShader);
            SAFE_DELETE(pComputePipeline);

            // Reset the image
            ResetImage();

            bIsCompiling = false;
            return true;
        });
    }
}

void CSoftwareRayTracer::UpdateGlobalBuffers(CCommandBuffer* pCommandBuffer)
{
    // Update GPU buffers
    if (m_pScene->m_bUpdateBuffers && m_pScene->m_pVertexPositionsBuffer)
    {
        VkBufferCopy BufferCopy;
        BufferCopy.size      = m_pScene->m_pVertexPositionsBuffer->GetSize();
        BufferCopy.dstOffset = 0;
        BufferCopy.srcOffset = 0;

        assert(m_pVertexPositionsBuffer->GetSize() >= m_pScene->m_pVertexPositionsBuffer->GetSize());
        pCommandBuffer->CopyBuffer(m_pScene->m_pVertexPositionsBuffer->GetBuffer(), m_pVertexPositionsBuffer->GetBuffer(), 1, &BufferCopy);
    }

    if (m_pScene->m_bUpdateBuffers && m_pScene->m_pVertexBuffer)
    {
        VkBufferCopy BufferCopy;
        BufferCopy.size      = m_pScene->m_pVertexBuffer->GetSize();
        BufferCopy.dstOffset = 0;
        BufferCopy.srcOffset = 0;

        assert(m_pVertexBuffer->GetSize() >= m_pScene->m_pVertexBuffer->GetSize());
        pCommandBuffer->CopyBuffer(m_pScene->m_pVertexBuffer->GetBuffer(), m_pVertexBuffer->GetBuffer(), 1, &BufferCopy);
    }

    if (m_pScene->m_bUpdateBuffers && m_pScene->m_pIndexBuffer)
    {
        VkBufferCopy BufferCopy;
        BufferCopy.size      = m_pScene->m_pIndexBuffer->GetSize();
        BufferCopy.dstOffset = 0;
        BufferCopy.srcOffset = 0;

        assert(m_pIndexBuffer->GetSize() >= m_pScene->m_pIndexBuffer->GetSize());
        pCommandBuffer->CopyBuffer(m_pScene->m_pIndexBuffer->GetBuffer(), m_pIndexBuffer->GetBuffer(), 1, &BufferCopy);
    }

    if (m_pScene->m_bUpdateBuffers && m_pScene->m_pTriangleBuffer)
    {
        VkBufferCopy BufferCopy;
        BufferCopy.size      = m_pScene->m_pTriangleBuffer->GetSize();
        BufferCopy.dstOffset = 0;
        BufferCopy.srcOffset = 0;

        assert(m_pTriangleBuffer->GetSize() >= m_pScene->m_pTriangleBuffer->GetSize());
        pCommandBuffer->CopyBuffer(m_pScene->m_pTriangleBuffer->GetBuffer(), m_pTriangleBuffer->GetBuffer(), 1, &BufferCopy);
    }

    if (m_pScene->m_bUpdateBuffers && m_pScene->m_pBoundingBoxBuffer)
    {
        VkBufferCopy BufferCopy;
        BufferCopy.size      = m_pScene->m_pBoundingBoxBuffer->GetSize();
        BufferCopy.dstOffset = 0;
        BufferCopy.srcOffset = 0;

        assert(m_pBvhBuffer->GetSize() >= m_pScene->m_pBoundingBoxBuffer->GetSize());
        pCommandBuffer->CopyBuffer(m_pScene->m_pBoundingBoxBuffer->GetBuffer(), m_pBvhBuffer->GetBuffer(), 1, &BufferCopy);
    }

    if (m_pScene->m_bUpdateBuffers && m_pScene->m_pAABBInstanceBuffer)
    {
        VkBufferCopy BufferCopy;
        BufferCopy.size      = m_pScene->m_pAABBInstanceBuffer->GetSize();
        BufferCopy.dstOffset = 0;
        BufferCopy.srcOffset = 0;

        assert(m_pAABBInstanceBuffer->GetSize() >= m_pScene->m_pAABBInstanceBuffer->GetSize());
        pCommandBuffer->CopyBuffer(m_pScene->m_pAABBInstanceBuffer->GetBuffer(), m_pAABBInstanceBuffer->GetBuffer(), 1, &BufferCopy);
    }

    // Do not update next frame
    m_pScene->m_bUpdateBuffers = false;

    // Update smaller buffers
    if (!m_pScene->m_Quads.empty())
    {
        pCommandBuffer->FillBuffer(m_pQuadBuffer, 0, m_pQuadBuffer->GetSize(), 0);
        assert((sizeof(SQuadGLSL) * m_pScene->m_Quads.size()) <= m_pQuadBuffer->GetSize());
        pCommandBuffer->UpdateBuffer(m_pQuadBuffer, 0, sizeof(SQuadGLSL) * m_pScene->m_Quads.size(), m_pScene->m_Quads.data());
    }

    if (!m_pScene->m_Spheres.empty())
    {
        pCommandBuffer->FillBuffer(m_pSphereBuffer, 0, m_pSphereBuffer->GetSize(), 0);
        assert((sizeof(SSphereGLSL) * m_pScene->m_Spheres.size()) <= m_pSphereBuffer->GetSize());
        pCommandBuffer->UpdateBuffer(m_pSphereBuffer, 0, sizeof(SSphereGLSL) * m_pScene->m_Spheres.size(), m_pScene->m_Spheres.data());
    }

    if (!m_pScene->m_Meshes.empty())
    {
        pCommandBuffer->FillBuffer(m_pMeshBuffer, 0, m_pMeshBuffer->GetSize(), 0);
        assert((sizeof(SMeshGLSL) * m_pScene->m_Meshes.size()) <= m_pMeshBuffer->GetSize());
        pCommandBuffer->UpdateBuffer(m_pMeshBuffer, 0, sizeof(SMeshGLSL) * m_pScene->m_Meshes.size(), m_pScene->m_Meshes.data());
    }

    if (!m_pScene->m_GpuMaterials.empty())
    {
        pCommandBuffer->FillBuffer(m_pMaterialBuffer, 0, m_pMaterialBuffer->GetSize(), 0);
        assert((sizeof(SMaterialGLSL) * m_pScene->m_GpuMaterials.size()) <= m_pMaterialBuffer->GetSize());
        pCommandBuffer->UpdateBuffer(m_pMaterialBuffer, 0, sizeof(SMaterialGLSL) * m_pScene->m_GpuMaterials.size(), m_pScene->m_GpuMaterials.data());
    }

    // Barrier before reading the buffer from the shader
    VkMemoryBarrier MemoryBarrier;
    MemoryBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    MemoryBarrier.pNext         = nullptr;
    MemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    MemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    pCommandBuffer->PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &MemoryBarrier, 0, nullptr, 0, nullptr);
}
