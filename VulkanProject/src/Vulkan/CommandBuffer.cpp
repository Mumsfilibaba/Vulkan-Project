#include "CommandBuffer.h"
#include "Buffer.h"
#include "VulkanContext.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "PipelineState.h"

CommandBuffer* CommandBuffer::Create(VulkanContext* pContext, const CommandBufferParams& params)
{
    CommandBuffer* pCommandBuffer = new CommandBuffer(pContext->GetDevice());
    
    VkCommandPoolCreateInfo poolInfo;
    ZERO_STRUCT(&poolInfo);
    
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = pContext->GetQueueFamilyIndex(params.QueueType);

    VkResult result = vkCreateCommandPool(pCommandBuffer->m_Device, &poolInfo, nullptr, &pCommandBuffer->m_CommandPool);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkCreateCommandPool failed. Error: " << result << std::endl;
        return nullptr;
    }
    else
    {
        std::cout << "Created commandpool" << std::endl;
    }

    VkCommandBufferAllocateInfo allocInfo = {};
    ZERO_STRUCT(&allocInfo);
    
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = pCommandBuffer->m_CommandPool;
    allocInfo.level              = params.Level;
    allocInfo.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(pCommandBuffer->m_Device, &allocInfo, &pCommandBuffer->m_CommandBuffer);
    if (result != VK_SUCCESS) 
    {
        std::cout << "vkAllocateCommandBuffers failed. Error: " << result << std::endl;
        return nullptr;
    }
    else
    {
        std::cout << "Allocated CommandBuffer" << std::endl;
    }

    VkFenceCreateInfo fenceInfo;
    ZERO_STRUCT(&fenceInfo);
    
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    result = vkCreateFence(pCommandBuffer->m_Device, &fenceInfo, nullptr, &pCommandBuffer->m_Fence);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkCreateFence failed. Error: " << result << std::endl;
        return nullptr;
    }
    else
    {
        std::cout << "Created Fence for commandbuffer" << std::endl;
    }
    
    return pCommandBuffer;
}

CommandBuffer::CommandBuffer(VkDevice device)
    : m_Device(device)
    , m_CommandPool(VK_NULL_HANDLE)
    , m_CommandBuffer(VK_NULL_HANDLE)
    , m_Fence(VK_NULL_HANDLE)
{
}

CommandBuffer::~CommandBuffer()
{
    if (m_Fence != VK_NULL_HANDLE)
    {
        WaitForAndResetFences();
        
        vkDestroyFence(m_Device, m_Fence, nullptr);
        m_Fence = VK_NULL_HANDLE;
    }

    if (m_CommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
        m_CommandPool = VK_NULL_HANDLE;
    }

    m_Device = VK_NULL_HANDLE;
}

void CommandBuffer::TransitionImage(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier barrier;
    ZERO_STRUCT(&barrier);
    
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = oldLayout;
    barrier.newLayout                       = newLayout;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    {
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = 0;

        sourceStage      = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage      = VK_PIPELINE_STAGE_HOST_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL)
    {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        sourceStage      = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage      = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        std::cout << "unsupported layout transition!" << std::endl;
        assert(false);
        return;
    }

    vkCmdPipelineBarrier(m_CommandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}
