#include "RayTracer.h"
#include "GUI.h"
#include "Vulkan/PipelineState.h"
#include "Vulkan/ShaderModule.h"
#include "Vulkan/DescriptorSetLayout.h"
#include "Vulkan/PipelineLayout.h"

FRayTracer::FRayTracer()
    : FBaseRenderer()
    , m_pScene(nullptr)
    , m_pRayTracingPipeline(nullptr)
    , m_pRayTracingPipelineLayout(nullptr)
    , m_pRayTracingDescriptorSetLayout(nullptr)
{
}

FRayTracer::~FRayTracer()
{
}

void FRayTracer::CreateResources()
{
    m_pScene = new FScene();
    m_pScene->Initialize();

    // Create RayTracing DescriptorSetLayout
    constexpr uint32_t NumRayTracingBindings = 3;
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
    
    // Camera Buffer
    RayTracingBindings[2].binding            = 2;
    RayTracingBindings[2].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    RayTracingBindings[2].descriptorCount    = 1;
    RayTracingBindings[2].stageFlags         = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    RayTracingBindings[2].pImmutableSamplers = nullptr;

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
    SAFE_DELETE(m_pRayTracingPipeline);
    SAFE_DELETE(m_pRayTracingPipelineLayout);
    SAFE_DELETE(m_pRayTracingDescriptorSetLayout);
}

void FRayTracer::Render(FCommandBuffer* pCommandBuffer)
{
}

void FRayTracer::RenderSceneUI()
{
}

void FRayTracer::ReloadShaders()
{
}