#include "CommandBuffer.h"
#include "Buffer.h"
#include "VulkanContext.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "PipelineState.h"

CommandBuffer::CommandBuffer(VkDevice device)
    : m_Device(device),
    m_CommandPool(VK_NULL_HANDLE),
    m_CommandBuffer(VK_NULL_HANDLE),
    m_Fence(VK_NULL_HANDLE)
{
}

CommandBuffer::~CommandBuffer()
{
    if (m_Fence != VK_NULL_HANDLE)
    {
        vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, UINT64_MAX);

        vkDestroyFence(m_Device, m_Fence, nullptr);
        m_Fence = VK_NULL_HANDLE;
    }

    if (m_CommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
        m_CommandPool = VK_NULL_HANDLE;

        std::cout << "Destroyed CommandPool" << std::endl;
    }
}

CommandBuffer* CommandBuffer::Create(VulkanContext* pContext, const CommandBufferParams& params)
{
    CommandBuffer* newCommandBuffer = new CommandBuffer(pContext->GetDevice());
    
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.pNext = nullptr;
    poolInfo.flags = 0;
    poolInfo.queueFamilyIndex = pContext->GetQueueFamilyIndex(params.QueueType);

    VkResult result = vkCreateCommandPool(newCommandBuffer->m_Device, &poolInfo, nullptr, &newCommandBuffer->m_CommandPool);
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
    allocInfo.sType          = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.pNext          = nullptr;
    allocInfo.commandPool = newCommandBuffer->m_CommandPool;
    allocInfo.level          = params.Level;
    allocInfo.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(newCommandBuffer->m_Device, &allocInfo, &newCommandBuffer->m_CommandBuffer);
    if (result != VK_SUCCESS) 
    {
        std::cout << "vkAllocateCommandBuffers failed. Error: " << result << std::endl;
        return nullptr;
    }
    else
    {
        std::cout << "Allocated CommandBuffer" << std::endl;
    }

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = nullptr;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    result = vkCreateFence(newCommandBuffer->m_Device, &fenceInfo, nullptr, &newCommandBuffer->m_Fence);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkCreateFence failed. Error: " << result << std::endl;
        return nullptr;
    }
    else
    {
        std::cout << "Created Fence for commandbuffer" << std::endl;
    }
    
    return newCommandBuffer;
}
