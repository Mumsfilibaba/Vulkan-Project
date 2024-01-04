#include "RenderPass.h"
#include "Device.h"
#include <vector>

RenderPass* RenderPass::Create(Device* pDevice, const RenderPassParams &params)
{
    RenderPass* pRenderPass = new RenderPass(pDevice->GetDevice());
    
    std::vector<VkAttachmentReference>   colorAttachmentRefInfos;
    std::vector<VkAttachmentDescription> attachmentsInfos;

    VkAttachmentDescription colorAttachment;
    ZERO_STRUCT(&colorAttachment);
    
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    for (uint32_t i = 0; i < params.ColorAttachmentCount; i++)
    {
        colorAttachment.loadOp = params.pColorAttachments[i].LoadOp;
        if (colorAttachment.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
        {
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }

        colorAttachment.format  = params.pColorAttachments[i].Format;
        colorAttachment.storeOp = params.pColorAttachments[i].StoreOp;
        attachmentsInfos.push_back(colorAttachment);
        
        colorAttachmentRef.attachment = i;
        colorAttachmentRefInfos.push_back(colorAttachmentRef);
    }

    VkSubpassDescription subpass;
    ZERO_STRUCT(&subpass);
    
    subpass.inputAttachmentCount    = 0;
    subpass.pInputAttachments       = nullptr;
    subpass.pDepthStencilAttachment = nullptr;
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = uint32_t(colorAttachmentRefInfos.size());
    subpass.pColorAttachments       = colorAttachmentRefInfos.data();
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments    = nullptr;
    subpass.pResolveAttachments     = nullptr;

    VkSubpassDependency dependency;
    ZERO_STRUCT(&dependency);
    
    dependency.dependencyFlags = 0;
    dependency.srcSubpass      = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass      = 0;
    dependency.srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask   = 0;
    dependency.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo;
    ZERO_STRUCT(&renderPassInfo);
    
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies   = &dependency;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments    = &colorAttachment;
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;

    VkResult result = vkCreateRenderPass(pRenderPass->m_Device, &renderPassInfo, nullptr, &pRenderPass->m_RenderPass);
    if (result != VK_SUCCESS) 
    {
        std::cout << "vkCreateRenderPass failed\n";
        return nullptr;
    }
    else
    {
        std::cout << "Created RenderPass\n";
        return pRenderPass;
    }
}

RenderPass::RenderPass(VkDevice device)
    : m_Device(device)
    , m_RenderPass(VK_NULL_HANDLE)
{
}

RenderPass::~RenderPass()
{
    if (m_RenderPass)
    {
        vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
        m_RenderPass = VK_NULL_HANDLE;
    }

    m_Device = VK_NULL_HANDLE;
}
