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

FSampler*             FTextureResource::s_pCubeMapGenSampler             = nullptr;
FDescriptorSetLayout* FTextureResource::s_pCubeMapGenDescriptorSetLayout = nullptr;
FPipelineLayout*      FTextureResource::s_pCubeMapGenPipelineLayout      = nullptr;
FComputePipeline*     FTextureResource::s_pCubeMapGenPipelineState       = nullptr;

bool FTextureResource::InitLoader(FDevice* pDevice)
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

    FDescriptorSetLayoutParams DescriptorSetLayoutParams;
    DescriptorSetLayoutParams.pBindings   = Bindings;
    DescriptorSetLayoutParams.numBindings = NumBindings;

    s_pCubeMapGenDescriptorSetLayout = FDescriptorSetLayout::Create(pDevice, DescriptorSetLayoutParams);
    if (!s_pCubeMapGenDescriptorSetLayout)
    {
        std::cout << "Failed to create CubeMapGen DescriptorSetLayout\n";
        return false;
    }
    
    
    // Create PipelineLayout
    FPipelineLayoutParams PipelineLayoutParams;
    PipelineLayoutParams.ppLayouts        = &s_pCubeMapGenDescriptorSetLayout;
    PipelineLayoutParams.numLayouts       = 1;
    PipelineLayoutParams.numPushConstants = 1;
    
    s_pCubeMapGenPipelineLayout = FPipelineLayout::Create(pDevice, PipelineLayoutParams);
    if (!s_pCubeMapGenPipelineLayout)
    {
        std::cout << "Failed to create CubeMapGen PipelineLayout\n";
        return false;
    }

    // Create shader and pipeline
    FShaderModule* pComputeShader = FShaderModule::CreateFromFile(pDevice, "main", RESOURCE_PATH"/shaders/cubemapgen.spv");

    FComputePipelineStateParams PipelineParams = {};
    PipelineParams.pShader         = pComputeShader;
    PipelineParams.pPipelineLayout = s_pCubeMapGenPipelineLayout;
    
    s_pCubeMapGenPipelineState = FComputePipeline::Create(pDevice, PipelineParams);
    SAFE_DELETE(pComputeShader);
    
    if (!s_pCubeMapGenPipelineState)
    {
        std::cout << "Failed to create CubeMapGen Pipeline\n";
        return false;
    }
    
    // Sampler
    FSamplerParams SamplerParams = {};
    SamplerParams.MagFilter     = VK_FILTER_LINEAR;
    SamplerParams.MinFilter     = VK_FILTER_LINEAR;
    SamplerParams.MipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    SamplerParams.AddressModeU  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerParams.AddressModeV  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerParams.AddressModeW  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerParams.MinLod        = -1000;
    SamplerParams.MaxLod        = 1000;
    SamplerParams.MaxAnisotropy = 1.0f;
    
    s_pCubeMapGenSampler = FSampler::Create(pDevice, SamplerParams);
    if (!s_pCubeMapGenSampler)
    {
        std::cout << "Failed to create CubeMapGen Sampler\n";
        return false;
    }
    
    return true;
}

void FTextureResource::ReleaseLoader()
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


FTextureResource* FTextureResource::LoadFromFile(FDevice* pDevice, const char* Filepath)
{
    FILE* File = fopen(Filepath, "rb");
    if (!File)
    {
        std::cout << "Failed to open '" << Filepath << "'\n";
        return nullptr;
    }
    
    // Get the file size
    fseek(File, 0, SEEK_END);
    int32_t FileSize = ftell(File);
    rewind(File);
    
    std::vector<uint8_t> FileData;
    FileData.resize(FileSize);
    fread(FileData.data(), FileData.size(), sizeof(uint8_t), File);
    
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
        std::cout << "Failed to load '" << Filepath << "'\n";
        return nullptr;
    }

    // Texture
    FTextureParams TextureParams = {};
    TextureParams.Format        = Format;
    TextureParams.ImageType     = VK_IMAGE_TYPE_2D;
    TextureParams.Width         = Width;
    TextureParams.Height        = Height;
    TextureParams.Usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    TextureParams.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    std::unique_ptr<FTexture> pTexture = std::unique_ptr<FTexture>(FTexture::CreateWithData(pDevice, TextureParams, Pixels.get()));
    if (!pTexture)
    {
        std::cout << "Failed to create Texture '" << Filepath << "'\n";
        return nullptr;
    }
    else
    {
        SetDebugName(pDevice->GetDevice(), std::string("Texture '") + Filepath + "'", reinterpret_cast<uint64_t>(pTexture->GetImage()), VK_OBJECT_TYPE_IMAGE);
    }

    // TextureView
    FTextureViewParams TextureViewParams = {};
    TextureViewParams.pTexture = pTexture.get();

    std::unique_ptr<FTextureView> pTextureView = std::unique_ptr<FTextureView>(FTextureView::Create(pDevice, TextureViewParams));
    if (!pTextureView)
    {
        std::cout << "Failed to create TextureView '" << Filepath << "'\n";
        return nullptr;
    }
    else
    {
        SetDebugName(pDevice->GetDevice(), std::string("TextureView '") + Filepath + "'", reinterpret_cast<uint64_t>(pTextureView->GetImageView()), VK_OBJECT_TYPE_IMAGE_VIEW);
    }

    std::unique_ptr<FTextureResource> pTextureResource = std::make_unique<FTextureResource>(pDevice);
    pTextureResource->m_pTexture     = pTexture.release();
    pTextureResource->m_pTextureView = pTextureView.release();
    pTextureResource->m_Width        = Width;
    pTextureResource->m_Height       = Height;
    
    std::cout << "Loaded Texture '" << Filepath << "'\n";
    return pTextureResource.release();
}

FTextureResource* FTextureResource::LoadCubeMapFromPanoramaFile(FDevice* pDevice, const char* Filepath)
{
    std::unique_ptr<FTextureResource> pPanorama = std::unique_ptr<FTextureResource>(LoadFromFile(pDevice, Filepath));
    if (!pPanorama)
    {
        return nullptr;
    }
    
    // Texture
    constexpr uint32_t CubeMapSize = 1024;
    FTextureParams TextureParams = {};
    TextureParams.Format         = VK_FORMAT_R16G16B16A16_SFLOAT;
    TextureParams.ImageType      = VK_IMAGE_TYPE_2D;
    TextureParams.Flags          = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    TextureParams.Width          = CubeMapSize;
    TextureParams.Height         = CubeMapSize;
    TextureParams.NumArraySlices = 6;
    TextureParams.Usage          = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    
    std::unique_ptr<FTexture> pTexture = std::unique_ptr<FTexture>(FTexture::Create(pDevice, TextureParams));
    if (!pTexture)
    {
        std::cout << "Failed to create TextureCube '" << Filepath << "'\n";
        return nullptr;
    }
    else
    {
        SetDebugName(pDevice->GetDevice(), std::string("TextureCube '") + Filepath + "'", reinterpret_cast<uint64_t>(pTexture->GetImage()), VK_OBJECT_TYPE_IMAGE);
    }

    // TextureView
    FTextureViewParams TextureViewParams = {};
    TextureViewParams.pTexture       = pTexture.get();
    TextureViewParams.ViewType       = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    TextureViewParams.NumArraySlices = 6;
    
    std::unique_ptr<FTextureView> pTextureViewUAV = std::unique_ptr<FTextureView>(FTextureView::Create(pDevice, TextureViewParams));
    if (!pTextureViewUAV)
    {
        std::cout << "Failed to create TextureView UAV for TextureCube'" << Filepath << "'\n";
        return nullptr;
    }
    else
    {
        SetDebugName(pDevice->GetDevice(), std::string("TextureCubeView UAV'") + Filepath + "'", reinterpret_cast<uint64_t>(pTextureViewUAV->GetImageView()), VK_OBJECT_TYPE_IMAGE_VIEW);
    }
    
    TextureViewParams.pTexture       = pTexture.get();
    TextureViewParams.ViewType       = VK_IMAGE_VIEW_TYPE_CUBE;
    TextureViewParams.NumArraySlices = 6;
    
    std::unique_ptr<FTextureView> pTextureView = std::unique_ptr<FTextureView>(FTextureView::Create(pDevice, TextureViewParams));
    if (!pTextureView)
    {
        std::cout << "Failed to create TextureView for TextureCube'" << Filepath << "'\n";
        return nullptr;
    }
    else
    {
        SetDebugName(pDevice->GetDevice(), std::string("TextureCubeView '") + Filepath + "'", reinterpret_cast<uint64_t>(pTextureView->GetImageView()), VK_OBJECT_TYPE_IMAGE_VIEW);
    }
    
    // DescriptorPool
    FDescriptorPoolParams DescriptorPoolParams;
    DescriptorPoolParams.NumStorageImages         = 1;
    DescriptorPoolParams.NumCombinedImageSamplers = 1;
    DescriptorPoolParams.MaxSets                  = 1;
    
    std::unique_ptr<FDescriptorPool> pDescriptorPool = std::unique_ptr<FDescriptorPool>(FDescriptorPool::Create(pDevice, DescriptorPoolParams));
    if (!pDescriptorPool)
    {
        std::cout << "Failed to create DescriptorPool '" << Filepath << "'\n";
        return nullptr;
    }
    
    // DescriptorSet
    std::unique_ptr<FDescriptorSet> pDescriptorSet = std::unique_ptr<FDescriptorSet>(FDescriptorSet::Create(pDevice, pDescriptorPool.get(), s_pCubeMapGenDescriptorSetLayout));
    if (!pDescriptorSet)
    {
        std::cout << "Failed to create DescriptorSet '" << Filepath << "'\n";
        return nullptr;
    }
    else
    {
        pDescriptorSet->BindCombinedImageSampler(pPanorama->GetTextureView()->GetImageView(), s_pCubeMapGenSampler->GetSampler(), 0);
        pDescriptorSet->BindStorageImage(pTextureViewUAV->GetImageView(), 1);
    }
    
    // CommandBuffer
    FCommandBufferParams CommandBufferParams = {};
    CommandBufferParams.Level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferParams.QueueType = ECommandQueueType::Graphics;
    
    std::unique_ptr<FCommandBuffer> pCommandBuffer = std::unique_ptr<FCommandBuffer>(FCommandBuffer::Create(pDevice, CommandBufferParams));
    if (!pCommandBuffer)
    {
        std::cout << "Failed to create CommandBuffer '" << Filepath << "'\n";
        return nullptr;
    }
    
    pCommandBuffer->Reset();
    pCommandBuffer->Begin();
    
    pCommandBuffer->TransitionImage(pTexture->GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    
    struct FPushConstants
    {
        uint32_t CubeSize;
    } PushConstants;
    PushConstants.CubeSize = CubeMapSize;
    
    pCommandBuffer->PushConstants(s_pCubeMapGenPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(FPushConstants), &PushConstants);
    
    pCommandBuffer->BindComputePipelineState(s_pCubeMapGenPipelineState);
    pCommandBuffer->BindComputeDescriptorSet(s_pCubeMapGenPipelineLayout, pDescriptorSet.get());
    
    constexpr uint32_t NumThreadGroups = CubeMapSize / 16;
    pCommandBuffer->Dispatch(NumThreadGroups, NumThreadGroups, 6);
    
    pCommandBuffer->TransitionImage(pTexture->GetImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    pCommandBuffer->End();
    
    pDevice->ExecuteGraphics(pCommandBuffer.get(), nullptr, nullptr);
    pDevice->WaitForIdle();

    std::unique_ptr<FTextureResource> pTextureResource = std::make_unique<FTextureResource>(pDevice);
    pTextureResource->m_pTexture     = pTexture.release();
    pTextureResource->m_pTextureView = pTextureView.release();
    pTextureResource->m_Width        = pTextureResource->m_pTexture->GetWidth();
    pTextureResource->m_Height       = pTextureResource->m_pTexture->GetHeight();
    
    std::cout << "Loaded Texture '" << Filepath << "'\n";
    return pTextureResource.release();
}

FTextureResource::FTextureResource(FDevice* pDevice)
    : m_pDevice(pDevice)
    , m_pTexture(nullptr)
    , m_pTextureView(nullptr)
{
}

FTextureResource::~FTextureResource()
{
    SAFE_DELETE(m_pTexture);
    SAFE_DELETE(m_pTextureView);
}
