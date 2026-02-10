#include "Framebuffer.h"
#include "RenderPass.h"
#include "Device.h"
#include "Extensions.h"

CFramebuffer* CFramebuffer::Create(CDevice* pDevice, const SFramebufferParams& Params)
{
    CFramebuffer* pFramebuffer = new CFramebuffer(pDevice);
    
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
        LOG("vkCreateFramebuffer failed\n");
        return nullptr;
    }
    else
    {
        LOG("Created Framebuffer\n");

        pFramebuffer->m_Width  = Params.Width;
        pFramebuffer->m_Height = Params.Height;
    }
    
    return pFramebuffer;
}

CFramebuffer::CFramebuffer(CDevice* pDevice)
    : CDeviceChild(pDevice)
    , m_Framebuffer(VK_NULL_HANDLE)
{
}

CFramebuffer::~CFramebuffer()
{
    if (m_Framebuffer != VK_NULL_HANDLE)
    {
        vkDestroyFramebuffer(GetDevice()->GetDevice(), m_Framebuffer, nullptr);
        m_Framebuffer = VK_NULL_HANDLE;
    }
}

void CFramebuffer::SetDebugName(const char* DebugName)
{
    if (Extensions::vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT DebugNameInfo;
        ZERO_STRUCT(&DebugNameInfo);

        DebugNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        DebugNameInfo.objectType   = VK_OBJECT_TYPE_FRAMEBUFFER;
        DebugNameInfo.pObjectName  = DebugName;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_Framebuffer);

        VkResult Result = Extensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            LOG("Failed to set name '%s'. Error: %d\n", DebugNameInfo.pObjectName, Result);
        }
    }
}
