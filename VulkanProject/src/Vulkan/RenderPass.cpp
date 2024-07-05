#include "RenderPass.h"
#include "Device.h"
#include <vector>

FRenderPass* FRenderPass::Create(FDevice* pDevice, const FRenderPassParams& Params)
{
    FRenderPass* pRenderPass = new FRenderPass(pDevice);
    
    std::vector<VkAttachmentReference>   ColorAttachmentRefInfos;
    std::vector<VkAttachmentDescription> AttachmentsInfos;

    VkAttachmentDescription ColorAttachment;
    ZERO_STRUCT(&ColorAttachment);
    
    ColorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    ColorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    
    VkAttachmentReference ColorAttachmentRef = {};
    ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    for (uint32_t i = 0; i < Params.ColorAttachmentCount; i++)
    {
        ColorAttachment.format        = Params.pColorAttachments[i].Format;
        ColorAttachment.loadOp        = Params.pColorAttachments[i].LoadOp;
        ColorAttachment.storeOp       = Params.pColorAttachments[i].StoreOp;
        ColorAttachment.initialLayout = Params.pColorAttachments[i].InitialLayout;
        ColorAttachment.finalLayout   = Params.pColorAttachments[i].FinalLayout;
        AttachmentsInfos.push_back(ColorAttachment);
        
        ColorAttachmentRef.attachment = i;
        ColorAttachmentRefInfos.push_back(ColorAttachmentRef);
    }

    VkSubpassDescription Subpass;
    ZERO_STRUCT(&Subpass);
    
    Subpass.inputAttachmentCount    = 0;
    Subpass.pInputAttachments       = nullptr;
    Subpass.pDepthStencilAttachment = nullptr;
    Subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    Subpass.colorAttachmentCount    = uint32_t(ColorAttachmentRefInfos.size());
    Subpass.pColorAttachments       = ColorAttachmentRefInfos.data();
    Subpass.preserveAttachmentCount = 0;
    Subpass.pPreserveAttachments    = nullptr;
    Subpass.pResolveAttachments     = nullptr;

    VkSubpassDependency Dependency;
    ZERO_STRUCT(&Dependency);
    
    Dependency.dependencyFlags = 0;
    Dependency.srcSubpass      = VK_SUBPASS_EXTERNAL;
    Dependency.dstSubpass      = 0;
    Dependency.srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    Dependency.srcAccessMask   = 0;
    Dependency.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    Dependency.dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo RenderPassCreateInfo;
    ZERO_STRUCT(&RenderPassCreateInfo);
    
    RenderPassCreateInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassCreateInfo.dependencyCount = 1;
    RenderPassCreateInfo.pDependencies   = &Dependency;
    RenderPassCreateInfo.attachmentCount = 1;
    RenderPassCreateInfo.pAttachments    = &ColorAttachment;
    RenderPassCreateInfo.subpassCount    = 1;
    RenderPassCreateInfo.pSubpasses      = &Subpass;

    VkResult Result = vkCreateRenderPass(pDevice->GetDevice(), &RenderPassCreateInfo, nullptr, &pRenderPass->m_RenderPass);
    if (Result != VK_SUCCESS)
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

FRenderPass::FRenderPass(FDevice* pDevice)
    : FDeviceChild(pDevice)
    , m_RenderPass(VK_NULL_HANDLE)
{
}

FRenderPass::~FRenderPass()
{
    if (m_RenderPass)
    {
        vkDestroyRenderPass(GetDevice()->GetDevice(), m_RenderPass, nullptr);
        m_RenderPass = VK_NULL_HANDLE;
    }
}
