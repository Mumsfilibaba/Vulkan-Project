#include "TextureResource.h"
#include "Vulkan/Helpers.h"
#include "Vulkan/Device.h"
#include "Vulkan/ShaderModule.h"
#include "Vulkan/DescriptorSetLayout.h"
#include "Vulkan/DescriptorSet.h"
#include "Vulkan/DescriptorPool.h"

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
    constexpr uint32_t numBindings = 2;
    VkDescriptorSetLayoutBinding bindings[numBindings];
    bindings[0].binding            = 0;
    bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount    = 1;
    bindings[0].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    bindings[1].binding            = 1;
    bindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].descriptorCount    = 1;
    bindings[1].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[1].pImmutableSamplers = nullptr;

    FDescriptorSetLayoutParams descriptorSetLayoutParams;
    descriptorSetLayoutParams.pBindings   = bindings;
    descriptorSetLayoutParams.numBindings = numBindings;

    s_pCubeMapGenDescriptorSetLayout = FDescriptorSetLayout::Create(pDevice, descriptorSetLayoutParams);
    if (!s_pCubeMapGenDescriptorSetLayout)
    {
        std::cout << "Failed to create CubeMapGen DescriptorSetLayout\n";
        return false;
    }
    
    
    // Create PipelineLayout
    FPipelineLayoutParams pipelineLayoutParams;
    pipelineLayoutParams.ppLayouts        = &s_pCubeMapGenDescriptorSetLayout;
    pipelineLayoutParams.numLayouts       = 1;
    pipelineLayoutParams.numPushConstants = 1;
    
    s_pCubeMapGenPipelineLayout = FPipelineLayout::Create(pDevice, pipelineLayoutParams);
    if (!s_pCubeMapGenPipelineLayout)
    {
        std::cout << "Failed to create CubeMapGen PipelineLayout\n";
        return false;
    }

    // Create shader and pipeline
    FShaderModule* pComputeShader = FShaderModule::CreateFromFile(pDevice, "main", RESOURCE_PATH"/shaders/cubemapgen.spv");

    FComputePipelineStateParams pipelineParams = {};
    pipelineParams.pShader         = pComputeShader;
    pipelineParams.pPipelineLayout = s_pCubeMapGenPipelineLayout;
    
    s_pCubeMapGenPipelineState = FComputePipeline::Create(pDevice, pipelineParams);
    SAFE_DELETE(pComputeShader);
    
    if (!s_pCubeMapGenPipelineState)
    {
        std::cout << "Failed to create CubeMapGen Pipeline\n";
        return false;
    }
    
    // Sampler
    FSamplerParams samplerParams = {};
    samplerParams.magFilter     = VK_FILTER_LINEAR;
    samplerParams.minFilter     = VK_FILTER_LINEAR;
    samplerParams.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerParams.addressModeU  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerParams.addressModeV  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerParams.addressModeW  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerParams.minLod        = -1000;
    samplerParams.maxLod        = 1000;
    samplerParams.maxAnisotropy = 1.0f;
    
    s_pCubeMapGenSampler = FSampler::Create(pDevice, samplerParams);
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

static VkFormat GetByteFormat(int32_t channels)
{
    if (channels == 4)
    {
        return VK_FORMAT_R8G8B8A8_UNORM;
    }
    else if (channels == 2)
    {
        return VK_FORMAT_R8G8_UNORM;
    }
    else if (channels == 1)
    {
        return VK_FORMAT_R8_UNORM;
    }
    else
    {
        return VK_FORMAT_UNDEFINED;
    }
}

static VkFormat GetExtendedFormat(int32_t channels)
{
    if (channels == 4)
    {
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    }
    else if (channels == 2)
    {
        return VK_FORMAT_R16G16_SFLOAT;
    }
    else if (channels == 1)
    {
        return VK_FORMAT_R16_SFLOAT;
    }
    else
    {
        return VK_FORMAT_UNDEFINED;
    }
}

static VkFormat GetFloatFormat(int32_t channels)
{
    if (channels == 4)
    {
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    }
    else if (channels == 3)
    {
        return VK_FORMAT_R32G32B32_SFLOAT;
    }
    else if (channels == 2)
    {
        return VK_FORMAT_R32G32_SFLOAT;
    }
    else if (channels == 1)
    {
        return VK_FORMAT_R32_SFLOAT;
    }
    else
    {
        return VK_FORMAT_UNDEFINED;
    }
}


FTextureResource* FTextureResource::LoadFromFile(FDevice* pDevice, const char* filepath)
{
    FILE* file = fopen(filepath, "rb");
    if (!file)
    {
        std::cout << "Failed to open '" << filepath << "'\n";
        return nullptr;
    }
    
    // Get the file size
    fseek(file, 0, SEEK_END);
    int32_t fileSize = ftell(file);
    rewind(file);
    
    std::vector<uint8_t> fileData;
    fileData.resize(fileSize);
    fread(fileData.data(), fileData.size(), sizeof(uint8_t), file);
    
    // Retrieve info about the file
    int32_t width        = 0;
    int32_t height       = 0;
    int32_t channelCount = 0;
    stbi_info_from_memory(fileData.data(), fileData.size(), &width, &height, &channelCount);

    const bool bIsFloat    = stbi_is_hdr_from_memory(fileData.data(), fileData.size());
    const bool bIsExtented = stbi_is_16_bit_from_memory(fileData.data(), fileData.size());

    VkFormat format = VK_FORMAT_UNDEFINED;
    
    // Load based on format
    std::unique_ptr<uint8_t[]> pixels;
    if (bIsExtented)
    {
        // We do not support 3 channel formats, force RGBA
        const auto numChannels = (channelCount == 3) ? 4 : channelCount;
        pixels = std::unique_ptr<uint8_t[]>(reinterpret_cast<uint8_t*>(stbi_load_16_from_memory(fileData.data(), fileData.size(), &width, &height, &channelCount, numChannels)));
        format = GetExtendedFormat(numChannels);
    }
    else if (bIsFloat)
    {
        // We do not support 3 channel formats, force RGBA (NOTE: Due to macOS for now, we might want to revisit this in the future,
        // but since we use these mostly for textures that we later convert into some other format, it should be fine)
        const auto numChannels = (channelCount == 3) ? 4 : channelCount;
        pixels = std::unique_ptr<uint8_t[]>(reinterpret_cast<uint8_t*>(stbi_loadf_from_memory(fileData.data(), fileData.size(), &width, &height, &channelCount, numChannels)));
        format = GetFloatFormat(numChannels);
    }
    else
    {
        // We do not support 3 channel formats, force RGBA
        const auto numChannels = (channelCount == 3) ? 4 : channelCount;
        pixels = std::unique_ptr<uint8_t[]>(stbi_load_from_memory(fileData.data(), fileData.size(), &width, &height, &channelCount, numChannels));
        format = GetByteFormat(numChannels);
    }

    assert(format != VK_FORMAT_UNDEFINED);
    if (!pixels)
    {
        std::cout << "Failed to load '" << filepath << "'\n";
        return nullptr;
    }

    // Texture
    FTextureParams textureParams = {};
    textureParams.Format        = format;
    textureParams.ImageType     = VK_IMAGE_TYPE_2D;
    textureParams.Width         = width;
    textureParams.Height        = height;
    textureParams.Usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    textureParams.InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    std::unique_ptr<FTexture> pTexture = std::unique_ptr<FTexture>(FTexture::CreateWithData(pDevice, textureParams, pixels.get()));
    if (!pTexture)
    {
        std::cout << "Failed to create Texture '" << filepath << "'\n";
        return nullptr;
    }
    else
    {
        SetDebugName(pDevice->GetDevice(), std::string("Texture '") + filepath + "'", reinterpret_cast<uint64_t>(pTexture->GetImage()), VK_OBJECT_TYPE_IMAGE);
    }

    // TextureView
    FTextureViewParams textureViewParams = {};
    textureViewParams.pTexture = pTexture.get();

    std::unique_ptr<FTextureView> pTextureView = std::unique_ptr<FTextureView>(FTextureView::Create(pDevice, textureViewParams));
    if (!pTextureView)
    {
        std::cout << "Failed to create TextureView '" << filepath << "'\n";
        return nullptr;
    }
    else
    {
        SetDebugName(pDevice->GetDevice(), std::string("TextureView '") + filepath + "'", reinterpret_cast<uint64_t>(pTextureView->GetImageView()), VK_OBJECT_TYPE_IMAGE_VIEW);
    }

    std::unique_ptr<FTextureResource> pTextureResource = std::make_unique<FTextureResource>(pDevice);
    pTextureResource->m_pTexture     = pTexture.release();
    pTextureResource->m_pTextureView = pTextureView.release();
    pTextureResource->m_Width        = width;
    pTextureResource->m_Height       = height;
    
    std::cout << "Loaded Texture '" << filepath << "'\n";
    return pTextureResource.release();
}

FTextureResource* FTextureResource::LoadCubeMapFromPanoramaFile(FDevice* pDevice, const char* filepath)
{
    std::unique_ptr<FTextureResource> pPanorama = std::unique_ptr<FTextureResource>(LoadFromFile(pDevice, filepath));
    if (!pPanorama)
    {
        return nullptr;
    }
    
    // Texture
    constexpr uint32_t CubeMapSize = 1024;
    FTextureParams textureParams = {};
    textureParams.Format         = VK_FORMAT_R16G16B16A16_SFLOAT;
    textureParams.ImageType      = VK_IMAGE_TYPE_2D;
    textureParams.Flags          = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    textureParams.Width          = CubeMapSize;
    textureParams.Height         = CubeMapSize;
    textureParams.NumArraySlices = 6;
    textureParams.Usage          = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    
    std::unique_ptr<FTexture> pTexture = std::unique_ptr<FTexture>(FTexture::Create(pDevice, textureParams));
    if (!pTexture)
    {
        std::cout << "Failed to create TextureCube '" << filepath << "'\n";
        return nullptr;
    }
    else
    {
        SetDebugName(pDevice->GetDevice(), std::string("TextureCube '") + filepath + "'", reinterpret_cast<uint64_t>(pTexture->GetImage()), VK_OBJECT_TYPE_IMAGE);
    }

    // TextureView
    FTextureViewParams textureViewParams = {};
    textureViewParams.pTexture       = pTexture.get();
    textureViewParams.ViewType       = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    textureViewParams.NumArraySlices = 6;
    
    std::unique_ptr<FTextureView> pTextureViewUAV = std::unique_ptr<FTextureView>(FTextureView::Create(pDevice, textureViewParams));
    if (!pTextureViewUAV)
    {
        std::cout << "Failed to create TextureView UAV for TextureCube'" << filepath << "'\n";
        return nullptr;
    }
    else
    {
        SetDebugName(pDevice->GetDevice(), std::string("TextureCubeView UAV'") + filepath + "'", reinterpret_cast<uint64_t>(pTextureViewUAV->GetImageView()), VK_OBJECT_TYPE_IMAGE_VIEW);
    }
    
    textureViewParams.pTexture       = pTexture.get();
    textureViewParams.ViewType       = VK_IMAGE_VIEW_TYPE_CUBE;
    textureViewParams.NumArraySlices = 6;
    
    std::unique_ptr<FTextureView> pTextureView = std::unique_ptr<FTextureView>(FTextureView::Create(pDevice, textureViewParams));
    if (!pTextureView)
    {
        std::cout << "Failed to create TextureView for TextureCube'" << filepath << "'\n";
        return nullptr;
    }
    else
    {
        SetDebugName(pDevice->GetDevice(), std::string("TextureCubeView '") + filepath + "'", reinterpret_cast<uint64_t>(pTextureView->GetImageView()), VK_OBJECT_TYPE_IMAGE_VIEW);
    }
    
    // DescriptorPool
    FDescriptorPoolParams poolParams;
    poolParams.NumStorageImages         = 1;
    poolParams.NumCombinedImageSamplers = 1;
    poolParams.MaxSets                  = 1;
    
    std::unique_ptr<FDescriptorPool> pDescriptorPool = std::unique_ptr<FDescriptorPool>(FDescriptorPool::Create(pDevice, poolParams));
    if (!pDescriptorPool)
    {
        std::cout << "Failed to create DescriptorPool '" << filepath << "'\n";
        return nullptr;
    }
    
    // DescriptorSet
    std::unique_ptr<FDescriptorSet> pDescriptorSet = std::unique_ptr<FDescriptorSet>(FDescriptorSet::Create(pDevice, pDescriptorPool.get(), s_pCubeMapGenDescriptorSetLayout));
    if (!pDescriptorSet)
    {
        std::cout << "Failed to create DescriptorSet '" << filepath << "'\n";
        return nullptr;
    }
    else
    {
        pDescriptorSet->BindCombinedImageSampler(pPanorama->GetTextureView()->GetImageView(), s_pCubeMapGenSampler->GetSampler(), 0);
        pDescriptorSet->BindStorageImage(pTextureViewUAV->GetImageView(), 1);
    }
    
    // CommandBuffer
    FCommandBufferParams commandBufferParams = {};
    commandBufferParams.Level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferParams.QueueType = ECommandQueueType::Graphics;
    
    std::unique_ptr<FCommandBuffer> pCommandBuffer = std::unique_ptr<FCommandBuffer>(FCommandBuffer::Create(pDevice, commandBufferParams));
    if (!pCommandBuffer)
    {
        std::cout << "Failed to create CommandBuffer '" << filepath << "'\n";
        return nullptr;
    }
    
    pCommandBuffer->Reset();
    pCommandBuffer->Begin();
    
    pCommandBuffer->TransitionImage(pTexture->GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    
    struct FPushConstants
    {
        uint32_t CubeSize;
    } pushConstants;
    pushConstants.CubeSize = CubeMapSize;
    
    pCommandBuffer->PushConstants(s_pCubeMapGenPipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(FPushConstants), &pushConstants);
    
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
    
    std::cout << "Loaded Texture '" << filepath << "'\n";
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
