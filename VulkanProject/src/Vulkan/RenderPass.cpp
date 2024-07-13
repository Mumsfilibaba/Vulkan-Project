#include "RenderPass.h"
#include "Device.h"
#include "Extensions.h"

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
    Subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    Subpass.colorAttachmentCount    = uint32_t(ColorAttachmentRefInfos.size());
    Subpass.pColorAttachments       = ColorAttachmentRefInfos.data();
    Subpass.preserveAttachmentCount = 0;
    Subpass.pPreserveAttachments    = nullptr;
    Subpass.pResolveAttachments     = nullptr;
    
    // Define depth attachment
    VkAttachmentReference DepthAttachmentRef = {};
    if (Params.pDepthAttachment)
    {
        VkAttachmentDescription DepthAttachment;
        ZERO_STRUCT(&DepthAttachment);

        DepthAttachment.format          = Params.pDepthAttachment->Format;  // Set the depth format (e.g., VK_FORMAT_D32_SFLOAT)
        DepthAttachment.samples         = VK_SAMPLE_COUNT_1_BIT;
        DepthAttachment.loadOp          = Params.pDepthAttachment->LoadOp;
        DepthAttachment.storeOp         = Params.pDepthAttachment->StoreOp;
        DepthAttachment.stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        DepthAttachment.stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        DepthAttachment.initialLayout   = Params.pDepthAttachment->InitialLayout; // Typically VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        DepthAttachment.finalLayout     = Params.pDepthAttachment->FinalLayout;   // Typically VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL

        DepthAttachmentRef.attachment = static_cast<uint32_t>(AttachmentsInfos.size());
        DepthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        AttachmentsInfos.push_back(DepthAttachment);
        
        Subpass.pDepthStencilAttachment = &DepthAttachmentRef;
    }
    else
    {
        Subpass.pDepthStencilAttachment = nullptr;
    }

    VkSubpassDependency Dependency;
    ZERO_STRUCT(&Dependency);

    Dependency.dependencyFlags = 0;
    Dependency.srcSubpass      = VK_SUBPASS_EXTERNAL;
    Dependency.dstSubpass      = 0;
    Dependency.srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    Dependency.srcAccessMask   = 0;
    Dependency.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    Dependency.dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo RenderPassCreateInfo;
    ZERO_STRUCT(&RenderPassCreateInfo);

    RenderPassCreateInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassCreateInfo.dependencyCount = 1;
    RenderPassCreateInfo.pDependencies   = &Dependency;
    RenderPassCreateInfo.attachmentCount = static_cast<uint32_t>(AttachmentsInfos.size());
    RenderPassCreateInfo.pAttachments    = AttachmentsInfos.data();
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

void FRenderPass::SetDebugName(const char* DebugName)
{
    if (FExtensions::vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT DebugNameInfo;
        ZERO_STRUCT(&DebugNameInfo);
        
        DebugNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        DebugNameInfo.objectType   = VK_OBJECT_TYPE_RENDER_PASS;
        DebugNameInfo.pObjectName  = DebugName;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_RenderPass);

        VkResult Result = FExtensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            std::cout << "Failed to set name '" << DebugNameInfo.pObjectName << "'.Error: " << Result << std::endl;
        }
    }
}