#include "RayTracer.h"
#include "GUI.h"
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
    , m_pMeshBuffer(nullptr)
    , m_pRayTracingPipeline(nullptr)
    , m_pRayTracingPipelineLayout(nullptr)
    , m_pRayTracingDescriptorSetLayout(nullptr)
    , m_pRayTracingDescriptorSet0(nullptr)
    , m_pRayTracingDescriptorSet1(nullptr)
    , m_pMaterialBuffer(nullptr)
{
}

FRayTracer::~FRayTracer()
{
}

void FRayTracer::CreateResources()
{
    m_pScene = new FScene(GetDevice());
    m_pScene->Initialize();

    // Create RayTracing DescriptorSetLayout
    constexpr uint32_t NumRayTracingBindings = 7;
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
    RayTracingBindings[3].stageFlags         = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    RayTracingBindings[3].pImmutableSamplers = nullptr;

    // RandomBuffer
    RayTracingBindings[4].binding            = 4;
    RayTracingBindings[4].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    RayTracingBindings[4].descriptorCount    = 1;
    RayTracingBindings[4].stageFlags         = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    RayTracingBindings[4].pImmutableSamplers = nullptr;

    // MeshBuffer
    RayTracingBindings[5].binding            = 5;
    RayTracingBindings[5].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[5].descriptorCount    = 1;
    RayTracingBindings[5].stageFlags         = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    RayTracingBindings[5].pImmutableSamplers = nullptr;

    // MaterialBuffer
    RayTracingBindings[6].binding            = 6;
    RayTracingBindings[6].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    RayTracingBindings[6].descriptorCount    = 1;
    RayTracingBindings[6].stageFlags         = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    RayTracingBindings[6].pImmutableSamplers = nullptr;

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

    FShaderModule* pRayClosestHitShader = FShaderModule::CreateFromFile(GetDevice(), "main", RESOURCE_PATH"/shaders/closesthit.spv");
    assert(pRayClosestHitShader != nullptr);
    pRayClosestHitShader->SetDebugName(RESOURCE_PATH"/shaders/closesthit.spv");

    FShaderModule* pRayMissShader = FShaderModule::CreateFromFile(GetDevice(), "main", RESOURCE_PATH"/shaders/miss.spv");
    assert(pRayMissShader != nullptr);
    pRayMissShader->SetDebugName(RESOURCE_PATH"/shaders/miss.spv");

    FRayTracingPipelineStateParams PipelineParams;
    PipelineParams.pRayGenShader        = pRayGenShader;
    PipelineParams.pRayClosestHitShader = pRayClosestHitShader;
    PipelineParams.pRayMissShader       = pRayMissShader;
    PipelineParams.pPipelineLayout      = m_pRayTracingPipelineLayout;

    m_pRayTracingPipeline = FRayTracingPipeline::Create(GetDevice(), PipelineParams);
    assert(m_pRayTracingPipeline != nullptr);

    SAFE_DELETE(pRayGenShader);
    SAFE_DELETE(pRayClosestHitShader);
    SAFE_DELETE(pRayMissShader);
}

void FRayTracer::ReleaseResources()
{
    SAFE_DELETE(m_pScene);
    SAFE_DELETE(m_pMeshBuffer);
    SAFE_DELETE(m_pRayTracingPipeline);
    SAFE_DELETE(m_pRayTracingPipelineLayout);
    SAFE_DELETE(m_pRayTracingDescriptorSetLayout);
    SAFE_DELETE(m_pMaterialBuffer);
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
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pMeshBuffer->GetBuffer(), 5);
    m_pRayTracingDescriptorSet0->BindStorageBuffer(m_pMaterialBuffer->GetBuffer(), 6);

    m_pRayTracingDescriptorSet1 = FDescriptorSet::Create(GetDevice(), GetDescriptorPool(), m_pRayTracingDescriptorSetLayout);
    assert(m_pRayTracingDescriptorSet1 != nullptr);
    m_pRayTracingDescriptorSet1->SetDebugName("RayTracingPass DescriptorSet1");

    m_pRayTracingDescriptorSet1->BindAccelerationStructure(m_pScene->m_pTopLevelAS->GetAccelerationStructure(), 0);
    m_pRayTracingDescriptorSet1->BindStorageImage(m_pSceneTextureView1->GetImageView(), 1);
    m_pRayTracingDescriptorSet1->BindStorageImage(m_pSceneTextureView0->GetImageView(), 2);
    m_pRayTracingDescriptorSet1->BindUniformBuffer(m_pCameraBuffer->GetBuffer(), 3);
    m_pRayTracingDescriptorSet1->BindUniformBuffer(m_pRandomBuffer->GetBuffer(), 4);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pMeshBuffer->GetBuffer(), 5);
    m_pRayTracingDescriptorSet1->BindStorageBuffer(m_pMaterialBuffer->GetBuffer(), 6);
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

void FRayTracer::RenderSceneUI()
{
}

void FRayTracer::ReloadShaders()
{
}

void FRayTracer::CreateGlobalBuffers()
{
    FBaseRenderer::CreateGlobalBuffers();

    FBufferParams MeshBufferParams = {};
    MeshBufferParams.Size             = m_pScene->m_MeshInfoBuffer.size() * sizeof(FMeshInfo);
    MeshBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    MeshBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;

    m_pMeshBuffer = FBuffer::CreateWithData(GetDevice(), MeshBufferParams, nullptr, m_pScene->m_MeshInfoBuffer.data());
    assert(m_pMeshBuffer != nullptr);
    m_pMeshBuffer->SetDebugName("MeshInfo-Buffer");

    // MaterialBuffer
    FBufferParams MaterialBufferParams;
    MaterialBufferParams.Size             = m_pScene->m_GpuMaterials.size() * sizeof(FShaderMaterial);
    MaterialBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    MaterialBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pMaterialBuffer = FBuffer::CreateWithData(GetDevice(), MaterialBufferParams, nullptr, m_pScene->m_GpuMaterials.data());
    assert(m_pMaterialBuffer != nullptr);
    m_pMaterialBuffer->SetDebugName("Material-Buffer");
}

void FRayTracer::UpdateGlobalBuffers(FCommandBuffer* pCommandBuffer)
{
    // Update GPU buffers
    if (!m_pScene->m_MeshInfoBuffer.empty())
    {
        pCommandBuffer->FillBuffer(m_pMeshBuffer, 0, m_pMeshBuffer->GetSize(), 0);
        assert(sizeof(FMeshInfo) * m_pScene->m_MeshInfoBuffer.size() < m_pMeshBuffer->GetSize());
        pCommandBuffer->UpdateBuffer(m_pMeshBuffer, 0, sizeof(FMeshInfo) * m_pScene->m_MeshInfoBuffer.size(), m_pScene->m_MeshInfoBuffer.data());
    }

    if (!m_pScene->m_GpuMaterials.empty())
    {
        pCommandBuffer->FillBuffer(m_pMaterialBuffer, 0, m_pMaterialBuffer->GetSize(), 0);
        assert(sizeof(FShaderMaterial) * m_pScene->m_GpuMaterials.size() < m_pMaterialBuffer->GetSize());
        pCommandBuffer->UpdateBuffer(m_pMaterialBuffer, 0, sizeof(FShaderMaterial) * m_pScene->m_GpuMaterials.size(), m_pScene->m_GpuMaterials.data());
    }

    // Barrier before reading the buffer from the shader
    VkMemoryBarrier MemoryBarrier;
    MemoryBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    MemoryBarrier.pNext         = nullptr;
    MemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    MemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    pCommandBuffer->PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 1, &MemoryBarrier, 0, nullptr, 0, nullptr);
}