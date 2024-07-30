#include "CommandBuffer.h"
#include "Buffer.h"
#include "Device.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "PipelineState.h"
#include "BindlessManager.h"
#include "Extensions.h"

FCommandBuffer* FCommandBuffer::Create(FDevice* pDevice, const FCommandBufferParams& Params)
{
    FCommandBuffer* pCommandBuffer = new FCommandBuffer(pDevice);
    
    VkCommandPoolCreateInfo CommandPoolInfo;
    ZERO_STRUCT(&CommandPoolInfo);
    
    CommandPoolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    CommandPoolInfo.queueFamilyIndex = pDevice->GetQueueFamilyIndex(Params.QueueType);

    VkResult Result = vkCreateCommandPool(pDevice->GetDevice(), &CommandPoolInfo, nullptr, &pCommandBuffer->m_CommandPool);
    if (Result != VK_SUCCESS)
    {
        LOG("vkCreateCommandPool failed. Error: %d\n", Result);
        return nullptr;
    }
    else
    {
        LOG("Created CommandPool\n");
    }

    VkCommandBufferAllocateInfo CommandBufferAllocateInfo = {};
    ZERO_STRUCT(&CommandBufferAllocateInfo);
    
    CommandBufferAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CommandBufferAllocateInfo.commandPool        = pCommandBuffer->m_CommandPool;
    CommandBufferAllocateInfo.level              = Params.Level;
    CommandBufferAllocateInfo.commandBufferCount = 1;

    Result = vkAllocateCommandBuffers(pDevice->GetDevice(), &CommandBufferAllocateInfo, &pCommandBuffer->m_CommandBuffer);
    if (Result != VK_SUCCESS)
    {
        LOG("vkAllocateCommandBuffers failed. Error: %d\n", Result);
        return nullptr;
    }
    else
    {
        LOG("Allocated CommandBuffer\n");
    }

    VkFenceCreateInfo FenceInfo;
    ZERO_STRUCT(&FenceInfo);
    
    FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    Result = vkCreateFence(pDevice->GetDevice(), &FenceInfo, nullptr, &pCommandBuffer->m_Fence);
    if (Result != VK_SUCCESS)
    {
        LOG("vkCreateFence failed. Error: %d\n", Result);
        return nullptr;
    }
    else
    {
        LOG("Created Fence for CommandBuffer\n");
    }
    
    return pCommandBuffer;
}

FCommandBuffer::FCommandBuffer(FDevice* pDevice)
    : FDeviceChild(pDevice)
    , m_CommandPool(VK_NULL_HANDLE)
    , m_CommandBuffer(VK_NULL_HANDLE)
    , m_Fence(VK_NULL_HANDLE)
    , m_NumCommands(0)
{
}

FCommandBuffer::~FCommandBuffer()
{
    if (m_Fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(GetDevice()->GetDevice(), m_Fence, nullptr);
        m_Fence = VK_NULL_HANDLE;
    }

    if (m_CommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(GetDevice()->GetDevice(), m_CommandPool, nullptr);
        m_CommandPool = VK_NULL_HANDLE;
    }
}

void FCommandBuffer::SetDebugName(const char* DebugName)
{
    if (FExtensions::vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT DebugNameInfo;
        ZERO_STRUCT(&DebugNameInfo);

        DebugNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        DebugNameInfo.objectType   = VK_OBJECT_TYPE_COMMAND_BUFFER;
        DebugNameInfo.pObjectName  = DebugName;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_CommandBuffer);

        VkResult Result = FExtensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            LOG("Failed to set name '%s'. Error: %d\n", DebugNameInfo.pObjectName, Result);
        }

        DebugNameInfo.objectType   = VK_OBJECT_TYPE_COMMAND_POOL;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_CommandPool);

        Result = FExtensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            LOG("Failed to set name '%s'. Error: %d\n", DebugNameInfo.pObjectName, Result);
        }

        DebugNameInfo.objectType   = VK_OBJECT_TYPE_FENCE;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_Fence);

        Result = FExtensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            LOG("Failed to set name '%s'. Error: %d\n", DebugNameInfo.pObjectName, Result);
        }
    }
}

void FCommandBuffer::BindBindlessDescriptors(FPipelineLayout* pPipelineLayout, VkPipelineBindPoint BindPoint)
{
    assert(pPipelineLayout != nullptr);
    assert(pPipelineLayout->GetBindlessDescriptorSetIndex() != uint32_t(-1));
    
    FBindlessManager& BindlessManager = GetDevice()->GetBindlessManager();
    VkDescriptorSet BindlessDescriptorSet = BindlessManager.GetDescriptorSet();
    vkCmdBindDescriptorSets(m_CommandBuffer, BindPoint, pPipelineLayout->GetPipelineLayout(), pPipelineLayout->GetBindlessDescriptorSetIndex(), 1, &BindlessDescriptorSet, 0, nullptr);
    m_NumCommands++;
}

void FCommandBuffer::TransitionImage(VkImage Image, VkImageLayout OldLayout, VkImageLayout NewLayout, VkImageAspectFlags AspectMask)
{
    VkImageMemoryBarrier Barrier;
    ZERO_STRUCT(&Barrier);
    
    Barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    Barrier.oldLayout                       = OldLayout;
    Barrier.newLayout                       = NewLayout;
    Barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    Barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    Barrier.image                           = Image;
    Barrier.subresourceRange.aspectMask     = AspectMask;
    Barrier.subresourceRange.baseMipLevel   = 0;
    Barrier.subresourceRange.levelCount     = 1;
    Barrier.subresourceRange.baseArrayLayer = 0;
    Barrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;

    VkPipelineStageFlags SourceStage;
    VkPipelineStageFlags DestinationStage;

    if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_GENERAL)
    {
        Barrier.srcAccessMask = 0;
        Barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        SourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DestinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    else if (OldLayout == VK_IMAGE_LAYOUT_GENERAL && NewLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    {
        Barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        Barrier.dstAccessMask = 0;

        SourceStage      = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        DestinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }
    else if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        Barrier.srcAccessMask = 0;
        Barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        SourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DestinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    else if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        Barrier.srcAccessMask = 0;
        Barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        SourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DestinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    }
    else if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        Barrier.srcAccessMask = 0;
        Barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        SourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DestinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        Barrier.srcAccessMask = 0;
        Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        SourceStage      = VK_PIPELINE_STAGE_HOST_BIT;
        DestinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        SourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
        DestinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (OldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_GENERAL)
    {
        Barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        Barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        SourceStage      = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        DestinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    else if (OldLayout == VK_IMAGE_LAYOUT_GENERAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        Barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        SourceStage      = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        DestinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        LOG("Unsupported layout transition!\n");
        assert(false);
        return;
    }

    vkCmdPipelineBarrier(m_CommandBuffer, SourceStage, DestinationStage, 0, 0, nullptr, 0, nullptr, 1, &Barrier);
    m_NumCommands++;
}
