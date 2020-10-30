#include "VulkanRenderPass.h"

#include <vector>

VulkanRenderPass::VulkanRenderPass(VkDevice device, const RenderPassParams& params)
    : m_Device(device),
    m_RenderPass(VK_NULL_HANDLE)
{
    Init(params);
}

VulkanRenderPass::~VulkanRenderPass()
{
    if (m_RenderPass)
    {
        vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
        m_RenderPass = VK_NULL_HANDLE;

        std::cout << "Destroyed RenderPass" << std::endl;
    }
}

void VulkanRenderPass::Init(const RenderPassParams& params)
{
    std::vector<VkAttachmentDescription> attachmentsInfos;
    std::vector<VkAttachmentReference> colorAttachmentRefInfos;

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.samples         = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp          = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp         = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    for (uint32_t i = 0; i < params.ColorAttachmentCount; i++)
    {
        colorAttachment.format = params.pColorAttachments[i].Format;
        attachmentsInfos.push_back(colorAttachment);
        
        colorAttachmentRef.attachment = i;
        colorAttachmentRefInfos.push_back(colorAttachmentRef);
    }

    VkSubpassDescription subpass = {};
    subpass.flags                   = 0;
    subpass.inputAttachmentCount    = 0;
    subpass.pInputAttachments       = nullptr;
    subpass.pDepthStencilAttachment = nullptr;
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = uint32_t(colorAttachmentRefInfos.size());
    subpass.pColorAttachments       = colorAttachmentRefInfos.data();
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments    = nullptr;
    subpass.pResolveAttachments     = nullptr;

    VkSubpassDependency dependency = {};
    dependency.dependencyFlags  = 0;
    dependency.srcSubpass       = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass       = 0;
    dependency.srcStageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask    = 0;
    dependency.dstStageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask    = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.flags = 0;
    renderPassInfo.pNext = nullptr;
    renderPassInfo.dependencyCount  = 1;
    renderPassInfo.pDependencies    = &dependency;
    renderPassInfo.attachmentCount  = 1;
    renderPassInfo.pAttachments     = &colorAttachment;
    renderPassInfo.subpassCount     = 1;
    renderPassInfo.pSubpasses       = &subpass;

    VkResult result = vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass);
    if (result != VK_SUCCESS) 
    {
        std::cout << "vkCreateRenderPass failed" << std::endl;
    }
    else
    {
        std::cout << "Created RenderPass" << std::endl;
    }
}
