#include "Texture.h"
#include "Device.h"
#include "Helpers.h"
#include "CommandBuffer.h"
#include "Buffer.h"

#include <algorithm>

static bool ValidateFormatForUpload(VkFormat Format)
{
    switch (Format)
    {
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return true;
    default:
        return false;
    }
}

static VkDeviceSize GetNumChannelsFromFormat(VkFormat Format)
{
    switch(Format)
    {
    case VK_FORMAT_R8_UNORM:
        return 1;
    case VK_FORMAT_R8G8_UNORM:
        return 2;
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return 4;
    default:
        return 0;
    }
}

static VkDeviceSize GetStrideFromFormat(VkFormat Format)
{
    switch(Format)
    {
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8B8A8_UNORM:
        return sizeof(char);
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return sizeof(float);
    default:
        return 0;
    }
}

CTexture* CTexture::Create(CDevice* pDevice, const STextureParams& Params)
{
    CTexture* pTexture = new CTexture(pDevice);

    VkImageCreateInfo TextureCreateInfo = {};
    ZERO_STRUCT(&TextureCreateInfo);

    TextureCreateInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    TextureCreateInfo.flags         = Params.Flags;
    TextureCreateInfo.imageType     = pTexture->m_ImageType = Params.ImageType;
    TextureCreateInfo.format        = pTexture->m_Format = Params.Format;
    TextureCreateInfo.extent.width  = pTexture->m_Width  = Params.Width;
    TextureCreateInfo.extent.height = pTexture->m_Height = Params.Height;
    TextureCreateInfo.extent.depth  = 1;
    TextureCreateInfo.mipLevels     = 1;
    TextureCreateInfo.arrayLayers   = pTexture->m_NumArraySlices = std::max(Params.NumArraySlices, 1u);
    TextureCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    TextureCreateInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    TextureCreateInfo.usage         = Params.Usage;
    TextureCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    TextureCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult Result = vkCreateImage(pDevice->GetDevice(), &TextureCreateInfo, nullptr, &pTexture->m_Image);
    if (Result != VK_SUCCESS)
    {
        LOG("vkCreateImage failed\n");
        return nullptr;
    }

    VkMemoryRequirements MemoryRequirements;
    vkGetImageMemoryRequirements(pDevice->GetDevice(), pTexture->m_Image, &MemoryRequirements);

    VkMemoryAllocateInfo MemoryAllocteInfo = {};
    ZERO_STRUCT(&MemoryAllocteInfo);

    MemoryAllocteInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    MemoryAllocteInfo.allocationSize  = MemoryRequirements.size;
    MemoryAllocteInfo.memoryTypeIndex = FindMemoryType(pDevice->GetPhysicalDevice(), MemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Result = vkAllocateMemory(pDevice->GetDevice(), &MemoryAllocteInfo, nullptr, &pTexture->m_Memory);
    if (Result != VK_SUCCESS)
    {
        LOG("vkAllocateMemory failed\n");
        return nullptr;
    }
    else
    {
        LOG("Allocated bytes\n", MemoryRequirements.size);
    }

    Result = vkBindImageMemory(pDevice->GetDevice(), pTexture->m_Image, pTexture->m_Memory, 0);
    if (Result != VK_SUCCESS)
    {
        LOG("vkAllocateMemory failed\n");
        return nullptr;
    }
    else
    {
        LOG("Created image w=%u, h=%u\n", Params.Width, Params.Height);
    }

    // Transfer image to the expected layout
    if (Params.InitialLayout != VK_IMAGE_LAYOUT_UNDEFINED)
    {
        SCommandBufferParams CommandBufferParams = {};
        CommandBufferParams.Level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        CommandBufferParams.QueueType = ECommandQueueType::Graphics;

        CCommandBuffer* pCommandBuffer = CCommandBuffer::Create(pDevice, CommandBufferParams);
        if (!pCommandBuffer)
        {
            SAFE_DELETE(pTexture);
            return nullptr;
        }
        else
        {
            pCommandBuffer->SetDebugName("CTexture::Create LayoutTransfer CommandBuffer");
        }

        pCommandBuffer->Reset();
        pCommandBuffer->Begin();

        VkImageAspectFlags AspectFlags;
        if (Params.Format == VK_FORMAT_D24_UNORM_S8_UINT)
        {
            AspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        else
        {
            AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        pCommandBuffer->TransitionImage(pTexture->m_Image, VK_IMAGE_LAYOUT_UNDEFINED, Params.InitialLayout, AspectFlags);
        pCommandBuffer->End();

        pDevice->ExecuteGraphics(pCommandBuffer, nullptr, nullptr);
        pDevice->WaitForIdle();

        SAFE_DELETE(pCommandBuffer);
    }

    return pTexture;
}

CTexture* CTexture::CreateWithData(CDevice* pDevice, const STextureParams& Params, const void* pSource)
{
    STextureParams ParamsCopy = Params;
    ParamsCopy.Usage         = ParamsCopy.Usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    ParamsCopy.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    CTexture* pTexture = CTexture::Create(pDevice, ParamsCopy);
    if (!pTexture)
    {
        return nullptr;
    }

    assert(ValidateFormatForUpload(Params.Format) == true);

    const VkDeviceSize NumChannels = GetNumChannelsFromFormat(Params.Format);
    const VkDeviceSize Stride      = GetStrideFromFormat(Params.Format);
    const VkDeviceSize UploadSize  = Params.Width * Params.Height * NumChannels * Stride;

    SBufferParams BufferParams = {};
    BufferParams.Usage            = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    BufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;
    BufferParams.Size             = UploadSize;

    CBuffer* pUploadBuffer = CBuffer::CreateWithData(pDevice, BufferParams, nullptr, pSource);
    if (!pUploadBuffer)
    {
        SAFE_DELETE(pTexture);
        return nullptr;
    }

    pUploadBuffer->SetDebugName("UploadBuffer");

    SCommandBufferParams CommandBufferParams = {};
    CommandBufferParams.Level     = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferParams.QueueType = ECommandQueueType::Graphics;

    CCommandBuffer* pCommandBuffer = CCommandBuffer::Create(pDevice, CommandBufferParams);
    if (!pCommandBuffer)
    {
        SAFE_DELETE(pUploadBuffer);
        SAFE_DELETE(pTexture);
        return nullptr;
    }
    else
    {
        pCommandBuffer->SetDebugName("CTexture::CreateWithData UploadCommandBuffer");
    }

    pCommandBuffer->Reset();
    pCommandBuffer->Begin();

    pCommandBuffer->TransitionImage(pTexture->GetImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

    VkBufferImageCopy BufferImageCopy = {};
    BufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    BufferImageCopy.imageSubresource.layerCount = 1;
    BufferImageCopy.imageExtent.width           = Params.Width;
    BufferImageCopy.imageExtent.height          = Params.Height;
    BufferImageCopy.imageExtent.depth           = 1;

    pCommandBuffer->CopyBufferToImage(pUploadBuffer->GetBuffer(), pTexture->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &BufferImageCopy);

    const VkImageLayout FinalLayout = (Params.InitialLayout == VK_IMAGE_LAYOUT_UNDEFINED) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : Params.InitialLayout;
    pCommandBuffer->TransitionImage(pTexture->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, FinalLayout, VK_IMAGE_ASPECT_COLOR_BIT);

    pCommandBuffer->End();

    pDevice->ExecuteGraphics(pCommandBuffer, nullptr, nullptr);
    pDevice->WaitForIdle();

    SAFE_DELETE(pUploadBuffer);
    SAFE_DELETE(pCommandBuffer);
    return pTexture;
}

CTexture::CTexture(CDevice* pDevice)
    : CDeviceChild(pDevice)
    , m_Image(VK_NULL_HANDLE)
    , m_Memory(VK_NULL_HANDLE)
    , m_Format(VK_FORMAT_UNDEFINED)
    , m_Width(0)
    , m_Height(0)
{
}

CTexture::~CTexture()
{
    if (m_Image != VK_NULL_HANDLE)
    {
        vkDestroyImage(GetDevice()->GetDevice(), m_Image, nullptr);
        m_Image = VK_NULL_HANDLE;
    }

    if (m_Memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(GetDevice()->GetDevice(), m_Memory, nullptr);
        m_Memory = VK_NULL_HANDLE;
    }
}

void CTexture::SetDebugName(const char* DebugName)
{
    if (Extensions::vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT DebugNameInfo;
        ZERO_STRUCT(&DebugNameInfo);

        DebugNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        DebugNameInfo.objectType   = VK_OBJECT_TYPE_IMAGE;
        DebugNameInfo.pObjectName  = DebugName;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_Image);

        VkResult Result = Extensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            LOG("Failed to set name '%s'. Error: %d\n", DebugNameInfo.pObjectName, Result);
        }

        DebugNameInfo.objectType   = VK_OBJECT_TYPE_DEVICE_MEMORY;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_Memory);

        Result = Extensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            LOG("Failed to set name '%s'. Error: %d\n", DebugNameInfo.pObjectName, Result);
        }
    }
}
