#include "Texture.h"
#include "Device.h"
#include "Helpers.h"
#include "CommandBuffer.h"
#include "Buffer.h"

FTexture* FTexture::Create(FDevice* pDevice, const FTextureParams& params)
{
    FTexture* pTexture = new FTexture(pDevice);
    
    VkImageCreateInfo textureCreateInfo = {};
    ZERO_STRUCT(&textureCreateInfo);
    
    textureCreateInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    textureCreateInfo.imageType     = params.ImageType;
    textureCreateInfo.format        = pTexture->m_Format = params.Format;
    textureCreateInfo.extent.width  = pTexture->m_Width  = params.Width;
    textureCreateInfo.extent.height = pTexture->m_Height = params.Height;
    textureCreateInfo.extent.depth  = 1;
    textureCreateInfo.mipLevels     = 1;
    textureCreateInfo.arrayLayers   = 1;
    textureCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    textureCreateInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    textureCreateInfo.usage         = params.Usage;
    textureCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    textureCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    
    VkResult result = vkCreateImage(pDevice->GetDevice(), &textureCreateInfo, nullptr, &pTexture->m_Image);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkCreateImage failed\n";
        return nullptr;
    }
    
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(pDevice->GetDevice(), pTexture->m_Image, &memoryRequirements);
    
    VkMemoryAllocateInfo memoryAllocteInfo = {};
    ZERO_STRUCT(&memoryAllocteInfo);
    
    memoryAllocteInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocteInfo.allocationSize  = memoryRequirements.size;
    memoryAllocteInfo.memoryTypeIndex = FindMemoryType(pDevice->GetPhysicalDevice(), memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    result = vkAllocateMemory(pDevice->GetDevice(), &memoryAllocteInfo, nullptr, &pTexture->m_Memory);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkAllocateMemory failed\n";
        return nullptr;
    }
    else
    {
        std::cout << "Allocated " << memoryRequirements.size << " bytes\n";
    }

    result = vkBindImageMemory(pDevice->GetDevice(), pTexture->m_Image, pTexture->m_Memory, 0);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkAllocateMemory failed\n";
        return nullptr;
    }
    else
    {
        std::cout << "Created image w=" << params.Width << ", h=" << params.Height << "\n";
    }

    // Transfer image to the expected layout
    if (params.InitialLayout != VK_IMAGE_LAYOUT_UNDEFINED)
    {
        FCommandBufferParams commandBufferParams = {};
        commandBufferParams.Level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferParams.QueueType = ECommandQueueType::Graphics;

        FCommandBuffer* pCommandBuffer = FCommandBuffer::Create(pDevice, commandBufferParams);
        pCommandBuffer->Reset();
        pCommandBuffer->Begin();
        pCommandBuffer->TransitionImage(pTexture->m_Image, VK_IMAGE_LAYOUT_UNDEFINED, params.InitialLayout);
        pCommandBuffer->End();

        pDevice->ExecuteGraphics(pCommandBuffer, nullptr, nullptr);
        pDevice->WaitForIdle();

        SAFE_DELETE(pCommandBuffer);
    }

    return pTexture;
}

FTexture* FTexture::CreateWithData(FDevice* pDevice, const FTextureParams& params, const void* pSource)
{
    FTextureParams paramsCopy = params;
    paramsCopy.Usage         = paramsCopy.Usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    paramsCopy.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    FTexture* pTexture = FTexture::Create(pDevice, paramsCopy);
    if (!pTexture)
    {
        return nullptr;
    }
    
    assert(params.Format == VK_FORMAT_R8G8B8A8_UNORM);
    VkDeviceSize uploadSize = params.Width * params.Height * 4 * sizeof(char);
    
    FBufferParams bufferParams = {};
    bufferParams.Usage            = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;
    bufferParams.Size             = uploadSize;
    
    FBuffer* pUploadBuffer = FBuffer::CreateWithData(pDevice, bufferParams, nullptr, pSource);
    if (!pUploadBuffer)
    {
        SAFE_DELETE(pTexture);
        return nullptr;
    }
    
    FCommandBufferParams commandBufferParams = {};
    commandBufferParams.Level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferParams.QueueType = ECommandQueueType::Graphics;
    
    FCommandBuffer* pCommandBuffer = FCommandBuffer::Create(pDevice, commandBufferParams);
    if (!pCommandBuffer)
    {
        SAFE_DELETE(pUploadBuffer);
        SAFE_DELETE(pTexture);
        return nullptr;
    }
    
    pCommandBuffer->Reset();
    pCommandBuffer->Begin();
    
    pCommandBuffer->TransitionImage(pTexture->GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    
    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width           = params.Width;
    region.imageExtent.height          = params.Height;
    region.imageExtent.depth           = 1;
    
    pCommandBuffer->CopyBufferToImage(pUploadBuffer->GetBuffer(), pTexture->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    const VkImageLayout finalLayout = (params.InitialLayout == VK_IMAGE_LAYOUT_UNDEFINED) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : params.InitialLayout;
    pCommandBuffer->TransitionImage(pTexture->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, finalLayout);
    
    pCommandBuffer->End();
    
    pDevice->ExecuteGraphics(pCommandBuffer, nullptr, nullptr);
    pDevice->WaitForIdle();
    
    SAFE_DELETE(pUploadBuffer);
    SAFE_DELETE(pCommandBuffer);
    return pTexture;
}

FTexture::FTexture(FDevice* pDevice)
    : m_pDevice(pDevice)
    , m_Image(VK_NULL_HANDLE)
    , m_Memory(VK_NULL_HANDLE)
    , m_Format(VK_FORMAT_UNDEFINED)
    , m_Width(0)
    , m_Height(0)
{
}

FTexture::~FTexture()
{
    VkDevice device = m_pDevice->GetDevice();

    if (m_Image != VK_NULL_HANDLE)
    {
        vkDestroyImage(device, m_Image, nullptr);
        m_Image = VK_NULL_HANDLE;
    }

    if (m_Memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, m_Memory, nullptr);
        m_Memory = VK_NULL_HANDLE;
    }
}
