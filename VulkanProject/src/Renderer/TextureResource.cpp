#include "TextureResource.h"
#include "Vulkan/Helpers.h"
#include "Vulkan/Device.h"
#include "Vulkan/ShaderModule.h"
#include "Vulkan/DescriptorSetLayout.h"
#include "Vulkan/DescriptorSet.h"
#include "Vulkan/DescriptorPool.h"
#include "Vulkan/CommandBuffer.h"
#include <memory>
#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/std_image.h"

CSampler*             CTextureResource::s_pCubeMapGenSampler             = nullptr;
CDescriptorSetLayout* CTextureResource::s_pCubeMapGenDescriptorSetLayout = nullptr;
CPipelineLayout*      CTextureResource::s_pCubeMapGenPipelineLayout      = nullptr;
CComputePipeline*     CTextureResource::s_pCubeMapGenPipelineState       = nullptr;

bool CTextureResource::InitLoader(CDevice* pDevice)
{
    // Create DescriptorSetLayout
    constexpr uint32_t NumBindings = 2;
    VkDescriptorSetLayoutBinding Bindings[NumBindings];
    Bindings[0].binding            = 0;
    Bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    Bindings[0].descriptorCount    = 1;
    Bindings[0].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    Bindings[0].pImmutableSamplers = nullptr;

    Bindings[1].binding            = 1;
    Bindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    Bindings[1].descriptorCount    = 1;
    Bindings[1].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    Bindings[1].pImmutableSamplers = nullptr;

    SDescriptorSetLayoutParams DescriptorSetLayoutParams;
    DescriptorSetLayoutParams.pBindings   = Bindings;
    DescriptorSetLayoutParams.NumBindings = NumBindings;

    s_pCubeMapGenDescriptorSetLayout = CDescriptorSetLayout::Create(pDevice, DescriptorSetLayoutParams);
    if (!s_pCubeMapGenDescriptorSetLayout)
    {
        LOG("Failed to create CubeMapGen DescriptorSetLayout\n");
        return false;
    }
    
    s_pCubeMapGenDescriptorSetLayout->SetDebugName("CubeMapGen DescriptorSetLayout");
    
    // Create PipelineLayout
    SPipelineLayoutParams PipelineLayoutParams;
    PipelineLayoutParams.ppLayouts        = &s_pCubeMapGenDescriptorSetLayout;
    PipelineLayoutParams.NumLayouts       = 1;
    PipelineLayoutParams.NumPushConstants = 1;
    
    s_pCubeMapGenPipelineLayout = CPipelineLayout::Create(pDevice, PipelineLayoutParams);
    if (!s_pCubeMapGenPipelineLayout)
    {
        LOG("Failed to create CubeMapGen PipelineLayout\n");
        return false;
    }

    s_pCubeMapGenPipelineLayout->SetDebugName("CubeMapGen PipelineLayout");

    // Create shader and pipeline
    CShaderModule* pComputeShader = CShaderModule::CreateFromFile(pDevice, "main", RESOURCE_PATH"/shaders/compiled_shaders/cubemapgen.spv");
    pComputeShader->SetDebugName(RESOURCE_PATH"/shaders/compiled_shaders/cubemapgen.spv");

    SComputePipelineStateParams PipelineParams = {};
    PipelineParams.pShader         = pComputeShader;
    PipelineParams.pPipelineLayout = s_pCubeMapGenPipelineLayout;
    
    s_pCubeMapGenPipelineState = CComputePipeline::Create(pDevice, PipelineParams);
    s_pCubeMapGenPipelineState->SetDebugName("CubeMapGen Pipeline");

    SAFE_DELETE(pComputeShader);

    if (!s_pCubeMapGenPipelineState)
    {
        LOG("Failed to create CubeMapGen Pipeline\n");
        return false;
    }
    
    // Sampler
    SSamplerParams SamplerParams = {};
    SamplerParams.MagFilter     = VK_FILTER_LINEAR;
    SamplerParams.MinFilter     = VK_FILTER_LINEAR;
    SamplerParams.MipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    SamplerParams.AddressModeU  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerParams.AddressModeV  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerParams.AddressModeW  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerParams.MinLod        = -1000;
    SamplerParams.MaxLod        = 1000;
    SamplerParams.MaxAnisotropy = 1.0f;
    
    s_pCubeMapGenSampler = CSampler::Create(pDevice, SamplerParams);
    if (!s_pCubeMapGenSampler)
    {
        LOG("Failed to create CubeMapGen Sampler\n");
        return false;
    }
    
    s_pCubeMapGenSampler->SetDebugName("CubeMapGenSampler");
    return true;
}

void CTextureResource::ReleaseLoader()
{
    SAFE_DELETE(s_pCubeMapGenSampler);
    SAFE_DELETE(s_pCubeMapGenDescriptorSetLayout);
    SAFE_DELETE(s_pCubeMapGenPipelineState);
    SAFE_DELETE(s_pCubeMapGenPipelineLayout);
}

static VkFormat GetByteFormat(int32_t Channels)
{
    if (Channels == 4)
    {
        return VK_FORMAT_R8G8B8A8_UNORM;
    }
    else if (Channels == 2)
    {
        return VK_FORMAT_R8G8_UNORM;
    }
    else if (Channels == 1)
    {
        return VK_FORMAT_R8_UNORM;
    }
    else
    {
        return VK_FORMAT_UNDEFINED;
    }
}

static VkFormat GetExtendedFormat(int32_t Channels)
{
    if (Channels == 4)
    {
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    }
    else if (Channels == 2)
    {
        return VK_FORMAT_R16G16_SFLOAT;
    }
    else if (Channels == 1)
    {
        return VK_FORMAT_R16_SFLOAT;
    }
    else
    {
        return VK_FORMAT_UNDEFINED;
    }
}

static VkFormat GetFloatFormat(int32_t Channels)
{
    if (Channels == 4)
    {
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    }
    else if (Channels == 3)
    {
        return VK_FORMAT_R32G32B32_SFLOAT;
    }
    else if (Channels == 2)
    {
        return VK_FORMAT_R32G32_SFLOAT;
    }
    else if (Channels == 1)
    {
        return VK_FORMAT_R32_SFLOAT;
    }
    else
    {
        return VK_FORMAT_UNDEFINED;
    }
}


CTextureResource* CTextureResource::LoadFromFile(CDevice* pDevice, const char* Filepath)
{
    FILE* File = fopen(Filepath, "rb");
    if (!File)
    {
        LOG("Failed to open '%s'\n", Filepath);
        return nullptr;
    }
    
    // Get the file size
    if (fseek(File, 0, SEEK_END) != 0)
    {
        fclose(File);
        LOG("Failed to seek '%s'\n", Filepath);
        return nullptr;
    }

    const long FileSizeLong = ftell(File);
    if (FileSizeLong <= 0)
    {
        fclose(File);
        LOG("Invalid file size for '%s'\n", Filepath);
        return nullptr;
    }

    const size_t FileSize = static_cast<size_t>(FileSizeLong);
    rewind(File);
    
    std::vector<uint8_t> FileData;
    FileData.resize(FileSize);
    const size_t NumReadBytes = fread(FileData.data(), sizeof(uint8_t), FileData.size(), File);
    fclose(File);
    if (NumReadBytes != FileData.size())
    {
        LOG("Failed to read '%s'\n", Filepath);
        return nullptr;
    }
    
    // Retrieve info about the file
    int32_t Width        = 0;
    int32_t Height       = 0;
    int32_t ChannelCount = 0;
    stbi_info_from_memory(FileData.data(), FileData.size(), &Width, &Height, &ChannelCount);

    const bool bIsFloat    = stbi_is_hdr_from_memory(FileData.data(), FileData.size());
    const bool bIsExtented = stbi_is_16_bit_from_memory(FileData.data(), FileData.size());

    VkFormat Format = VK_FORMAT_UNDEFINED;
    
    // Load based on format
    std::unique_ptr<uint8_t[]> Pixels;
    if (bIsExtented)
    {
        // We do not support 3 channel formats, force RGBA
        const auto NumChannels = (ChannelCount == 3) ? 4 : ChannelCount;
        Pixels = std::unique_ptr<uint8_t[]>(reinterpret_cast<uint8_t*>(stbi_load_16_from_memory(FileData.data(), FileData.size(), &Width, &Height, &ChannelCount, NumChannels)));
        Format = GetExtendedFormat(NumChannels);
    }
    else if (bIsFloat)
    {
        // We do not support 3 channel formats, force RGBA (NOTE: Due to macOS for now, we might want to revisit this in the future,
        // but since we use these mostly for textures that we later convert into some other format, it should be fine)
        const auto NumChannels = (ChannelCount == 3) ? 4 : ChannelCount;
        Pixels = std::unique_ptr<uint8_t[]>(reinterpret_cast<uint8_t*>(stbi_loadf_from_memory(FileData.data(), FileData.size(), &Width, &Height, &ChannelCount, NumChannels)));
        Format = GetFloatFormat(NumChannels);
    }
    else
    {
        // We do not support 3 channel formats, force RGBA
        const auto NumChannels = (ChannelCount == 3) ? 4 : ChannelCount;
        Pixels = std::unique_ptr<uint8_t[]>(stbi_load_from_memory(FileData.data(), FileData.size(), &Width, &Height, &ChannelCount, NumChannels));
        Format = GetByteFormat(NumChannels);
    }

    assert(Format != VK_FORMAT_UNDEFINED);

    if (!Pixels)
    {
        LOG("Failed to load '%s'\n", Filepath);
        return nullptr;
    }

    // Texture
    STextureParams TextureParams = {};
    TextureParams.Format        = Format;
    TextureParams.ImageType     = VK_IMAGE_TYPE_2D;
    TextureParams.Width         = Width;
    TextureParams.Height        = Height;
    TextureParams.Usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    TextureParams.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    std::unique_ptr<CTexture> pTexture = std::unique_ptr<CTexture>(CTexture::CreateWithData(pDevice, TextureParams, Pixels.get()));
    if (!pTexture)
    {
        LOG("Failed to create Texture '%s'\n", Filepath);
        return nullptr;
    }
    else
    {
        const std::string DebugName = std::string("Texture '") + Filepath + "'";
        pTexture->SetDebugName(DebugName.c_str());
    }

    // TextureView
    STextureViewParams TextureViewParams = {};
    TextureViewParams.pTexture = pTexture.get();

    std::unique_ptr<CTextureView> pTextureView = std::unique_ptr<CTextureView>(CTextureView::Create(pDevice, TextureViewParams));
    if (!pTextureView)
    {
        LOG("Failed to create TextureView '%s'\n", Filepath);
        return nullptr;
    }
    else
    {
        const std::string DebugName = std::string("TextureView '") + Filepath + "'";
        pTextureView->SetDebugName(DebugName.c_str());
    }

    std::unique_ptr<CTextureResource> pTextureResource = std::make_unique<CTextureResource>(pDevice);
    pTextureResource->m_pTexture     = pTexture.release();
    pTextureResource->m_pTextureView = pTextureView.release();
    pTextureResource->m_Width        = Width;
    pTextureResource->m_Height       = Height;

    LOG("Loaded Texture '%s'\n", Filepath);
    return pTextureResource.release();
}

CTextureResource* CTextureResource::LoadCubeMapFromPanoramaFile(CDevice* pDevice, const char* Filepath)
{
    std::unique_ptr<CTextureResource> pPanorama = std::unique_ptr<CTextureResource>(LoadFromFile(pDevice, Filepath));
    if (!pPanorama)
    {
        return nullptr;
    }
    
    // Texture
    constexpr uint32_t CubeMapSize = 1024;
    STextureParams TextureParams = {};
    TextureParams.Format         = VK_FORMAT_R16G16B16A16_SFLOAT;
    TextureParams.ImageType      = VK_IMAGE_TYPE_2D;
    TextureParams.Flags          = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    TextureParams.Width          = CubeMapSize;
    TextureParams.Height         = CubeMapSize;
    TextureParams.NumArraySlices = 6;
    TextureParams.Usage          = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    
    std::unique_ptr<CTexture> pTexture = std::unique_ptr<CTexture>(CTexture::Create(pDevice, TextureParams));
    if (!pTexture)
    {
        LOG("Failed to create TextureCube '%s'\n", Filepath);
        return nullptr;
    }
    else
    {
        const std::string DebugName = std::string("TextureCube '") + Filepath + "'";
        pTexture->SetDebugName(DebugName.c_str());
    }

    // TextureView
    STextureViewParams TextureViewParams = {};
    TextureViewParams.pTexture       = pTexture.get();
    TextureViewParams.ViewType       = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    TextureViewParams.NumArraySlices = 6;
    
    std::unique_ptr<CTextureView> pTextureViewUAV = std::unique_ptr<CTextureView>(CTextureView::Create(pDevice, TextureViewParams));
    if (!pTextureViewUAV)
    {
        LOG("Failed to create TextureView UAV for TextureCube '%s'\n", Filepath);
        return nullptr;
    }
    else
    {
        const std::string DebugName = std::string("TextureCubeView UAV '") + Filepath + "'";
        pTextureViewUAV->SetDebugName(DebugName.c_str());
    }
    
    TextureViewParams.pTexture       = pTexture.get();
    TextureViewParams.ViewType       = VK_IMAGE_VIEW_TYPE_CUBE;
    TextureViewParams.NumArraySlices = 6;
    
    std::unique_ptr<CTextureView> pTextureView = std::unique_ptr<CTextureView>(CTextureView::Create(pDevice, TextureViewParams));
    if (!pTextureView)
    {
        LOG("Failed to create TextureView for TextureCube '%s'\n", Filepath);
        return nullptr;
    }
    else
    {
        const std::string DebugName = std::string("TextureCubeView '") + Filepath + "'";
        pTextureViewUAV->SetDebugName(DebugName.c_str());
    }
    
    // DescriptorPool
    SDescriptorPoolParams DescriptorPoolParams;
    DescriptorPoolParams.NumStorageImages         = 1;
    DescriptorPoolParams.NumCombinedImageSamplers = 1;
    DescriptorPoolParams.MaxSets                  = 1;
    
    std::unique_ptr<CDescriptorPool> pDescriptorPool = std::unique_ptr<CDescriptorPool>(CDescriptorPool::Create(pDevice, DescriptorPoolParams));
    if (!pDescriptorPool)
    {
        LOG("Failed to create DescriptorPool '%s'\n", Filepath);
        return nullptr;
    }
    else
    {
        pDescriptorPool->SetDebugName("CubeMapGen DescriptorPool");
    }
    
    // DescriptorSet
    std::unique_ptr<CDescriptorSet> pDescriptorSet = std::unique_ptr<CDescriptorSet>(CDescriptorSet::Create(pDevice, pDescriptorPool.get(), s_pCubeMapGenDescriptorSetLayout));
    if (!pDescriptorSet)
    {
        LOG("Failed to create DescriptorSet '%s'\n", Filepath);
        return nullptr;
    }
    else
    {
        pDescriptorSet->SetDebugName("CubeMapGen DescriptorSet");
        pDescriptorSet->BindCombinedImageSampler(pPanorama->GetTextureView()->GetImageView(), s_pCubeMapGenSampler->GetSampler(), 0);
        pDescriptorSet->BindStorageImage(pTextureViewUAV->GetImageView(), 1);
    }
    
    // CommandBuffer
    SCommandBufferParams CommandBufferParams = {};
    CommandBufferParams.Level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferParams.QueueType = ECommandQueueType::Graphics;
    
    std::unique_ptr<CCommandBuffer> pCommandBuffer = std::unique_ptr<CCommandBuffer>(CCommandBuffer::Create(pDevice, CommandBufferParams));
    if (!pCommandBuffer)
    {
        LOG("Failed to create CommandBuffer '%s'\n", Filepath);
        return nullptr;
    }
    else
    {
        pCommandBuffer->SetDebugName("CTextureResource::LoadCubeMapFromPanoramaFile CommandBuffer");
    }
    
    pCommandBuffer->Reset();
    pCommandBuffer->Begin();
    
    pCommandBuffer->TransitionImage(pTexture->GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
    
    struct SPushConstants
    {
        uint32_t CubeSize;
    } PushConstants;
    PushConstants.CubeSize = CubeMapSize;
    
    pCommandBuffer->PushConstants(s_pCubeMapGenPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(SPushConstants), &PushConstants);
    
    pCommandBuffer->BindComputePipelineState(s_pCubeMapGenPipelineState);
    pCommandBuffer->BindComputeDescriptorSet(s_pCubeMapGenPipelineLayout, pDescriptorSet.get(), 0);
    
    constexpr uint32_t NumThreadGroups = CubeMapSize / 16;
    pCommandBuffer->Dispatch(NumThreadGroups, NumThreadGroups, 6);
    
    pCommandBuffer->TransitionImage(pTexture->GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    pCommandBuffer->End();
    
    pDevice->ExecuteGraphics(pCommandBuffer.get(), nullptr, nullptr);
    pDevice->WaitForIdle();

    std::unique_ptr<CTextureResource> pTextureResource = std::make_unique<CTextureResource>(pDevice);
    pTextureResource->m_pTexture     = pTexture.release();
    pTextureResource->m_pTextureView = pTextureView.release();
    pTextureResource->m_Width        = pTextureResource->m_pTexture->GetWidth();
    pTextureResource->m_Height       = pTextureResource->m_pTexture->GetHeight();

    LOG("Loaded Texture '%s'\n", Filepath);
    return pTextureResource.release();
}

CTextureResource::CTextureResource(CDevice* pDevice)
    : m_pDevice(pDevice)
    , m_pTexture(nullptr)
    , m_pTextureView(nullptr)
{
}

CTextureResource::~CTextureResource()
{
    SAFE_DELETE(m_pTexture);
    SAFE_DELETE(m_pTextureView);
}
