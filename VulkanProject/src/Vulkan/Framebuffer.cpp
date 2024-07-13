#include "Framebuffer.h"
#include "RenderPass.h"
#include "Device.h"

FFramebuffer* FFramebuffer::Create(FDevice* pDevice, const FFramebufferParams& Params)
{
    FFramebuffer* pFramebuffer = new FFramebuffer(pDevice);
    
    assert(Params.pRenderPass != nullptr);

    VkFramebufferCreateInfo FramebufferCreateInfo;
    ZERO_STRUCT(&FramebufferCreateInfo);
    
    FramebufferCreateInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FramebufferCreateInfo.renderPass      = Params.pRenderPass->GetRenderPass();
    FramebufferCreateInfo.attachmentCount = Params.AttachmentCount;
    FramebufferCreateInfo.pAttachments    = Params.pAttachMents;
    FramebufferCreateInfo.width           = Params.Width;
    FramebufferCreateInfo.height          = Params.Height;
    FramebufferCreateInfo.layers          = 1;

    VkResult Result = vkCreateFramebuffer(pDevice->GetDevice(), &FramebufferCreateInfo, nullptr, &pFramebuffer->m_Framebuffer);
    if (Result != VK_SUCCESS)
    {
        std::cout << "vkCreateFramebuffer failed\n";
        return nullptr;
    }
    else
    {
        std::cout << "Created Framebuffer\n";

        pFramebuffer->m_Width  = Params.Width;
        pFramebuffer->m_Height = Params.Height;
    }
    
    return pFramebuffer;
}

FFramebuffer::FFramebuffer(FDevice* pDevice)
    : FDeviceChild(pDevice)
    , m_Framebuffer(VK_NULL_HANDLE)
{
}

FFramebuffer::~FFramebuffer()
{
    if (m_Framebuffer != VK_NULL_HANDLE)
    {
        vkDestroyFramebuffer(GetDevice()->GetDevice(), m_Framebuffer, nullptr);
        m_Framebuffer = VK_NULL_HANDLE;
    }
}

void FFramebuffer::SetDebugName(const char* DebugName)
{
    if (FExtensions::vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT DebugNameInfo;
        ZERO_STRUCT(&DebugNameInfo);

        DebugNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        DebugNameInfo.objectType   = VK_OBJECT_TYPE_FRAMEBUFFER;
        DebugNameInfo.pObjectName  = DebugName;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_Framebuffer);

        VkResult Result = FExtensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            std::cout << "Failed to set name '" << DebugNameInfo.pObjectName << "'.Error: " << Result << std::endl;
        }
    }
}