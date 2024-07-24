#include "Swapchain.h"
#include "Device.h"
#include "Helpers.h"

#define NUM_BACK_BUFFERS (3)

FSwapchain* FSwapchain::Create(FDevice* pDevice, GLFWwindow* pWindow)
{
    FSwapchain* pSwapchain = new FSwapchain(pDevice, pWindow);
    if (!pSwapchain->CreateSurface())
    {
        return nullptr;
    }
    
    if (pSwapchain->CreateSemaphores())
    {
        LOG("Created Semphores and Fences\n");
    }
    else
    {
        return nullptr;
    }

    if (pSwapchain->CreateSwapchain())
    {
        LOG("Created Swapchain\n");
    }
    else
    {
        return nullptr;
    }
    
    return pSwapchain;
}

FSwapchain::FSwapchain(FDevice* pDevice, GLFWwindow* pWindow)
    : FDeviceChild(pDevice)
    , m_pWindow(pWindow)
    , m_Surface(VK_NULL_HANDLE)
    , m_Swapchain(VK_NULL_HANDLE)
    , m_Extent{ 0, 0 }
    , m_SwapchainFormat{ VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
    , m_PresentMode(VK_PRESENT_MODE_IMMEDIATE_KHR)
    , m_ImageCount(NUM_BACK_BUFFERS)
    , m_FrameData()
    , m_SemaphoreIndex(0)
    , m_CurrentBufferIndex(0)
{
    m_FrameData.resize(m_ImageCount);
}

FSwapchain::~FSwapchain()
{
    for (FSemaphores& SemaphoreData : m_SemaphoreData)
    {
        if (SemaphoreData.ImageSemaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(GetDevice()->GetDevice(), SemaphoreData.ImageSemaphore, nullptr);
            SemaphoreData.ImageSemaphore = VK_NULL_HANDLE;
        }
        
        if (SemaphoreData.RenderSemaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(GetDevice()->GetDevice(), SemaphoreData.RenderSemaphore, nullptr);
            SemaphoreData.RenderSemaphore = VK_NULL_HANDLE;
        }
    }
    
    ReleaseSwapchainResources();
    
    if (m_Surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(GetDevice()->GetInstance(), m_Surface, nullptr);
        m_Surface = VK_NULL_HANDLE;
    }
}

bool FSwapchain::CreateSurface()
{
    VkResult Result = glfwCreateWindowSurface(GetDevice()->GetInstance(), m_pWindow, nullptr, &m_Surface);
    if (Result != VK_SUCCESS)
    {
        LOG("glfwCreateWindowSurface failed  with error: %d\n", Result);
        return false;
    }

    return true;
}

bool FSwapchain::CreateSwapchain()
{
    int32_t Width  = 0;
    int32_t Height = 0;
    glfwGetFramebufferSize(m_pWindow, &Width, &Height);

    if (Width <= 0 || Height <= 0)
    {
        LOG("Width or Height is zero\n");
        return false;
    }

    // Get capabilities and formats that are supported
    VkSurfaceCapabilitiesKHR Capabilities = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(GetDevice()->GetPhysicalDevice(), m_Surface, &Capabilities);
    
    VkExtent2D NewExtent;
    if (Capabilities.currentExtent.width != UINT32_MAX && Capabilities.currentExtent.height != UINT32_MAX)
    {
        NewExtent = Capabilities.currentExtent;
    }
    else
    {
        VkExtent2D ActualExtent = { uint32_t(Width), uint32_t(Height) };
        ActualExtent.width  = std::max(Capabilities.minImageExtent.width,  std::min(Capabilities.maxImageExtent.width,  ActualExtent.width));
        ActualExtent.height = std::max(Capabilities.minImageExtent.height, std::min(Capabilities.maxImageExtent.height, ActualExtent.height));
        NewExtent = ActualExtent;
    }

    if (NewExtent.width <= 0 || NewExtent.height <= 0)
    {
        LOG("Extent contains a Width or Height that is zero\n");
        return false;
    }
    else
    {
        m_Extent = NewExtent;
    }

    std::vector<VkPresentModeKHR>   PresentModes;
    std::vector<VkSurfaceFormatKHR> Formats;

    uint32_t FormatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(GetDevice()->GetPhysicalDevice(), m_Surface, &FormatCount, nullptr);
    if (FormatCount > 0)
    {
        Formats.resize(FormatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(GetDevice()->GetPhysicalDevice(), m_Surface, &FormatCount, Formats.data());

        for (const auto& availableFormat : Formats)
        {
            if (availableFormat.format == VK_FORMAT_R16G16B16A16_SFLOAT && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                m_SwapchainFormat = availableFormat;
            }
            else if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                m_SwapchainFormat = availableFormat;
            }
        }
    }
    else
    {
        LOG("No available formats for Swapchain\n");
        return false;
    }

    uint32_t PresentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(GetDevice()->GetPhysicalDevice(), m_Surface, &PresentModeCount, nullptr);
    if (PresentModeCount > 0)
    {
        PresentModes.resize(PresentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(GetDevice()->GetPhysicalDevice(), m_Surface, &PresentModeCount, PresentModes.data());

        // Choose mailbox if available otherwise fifo
        m_PresentMode = VK_PRESENT_MODE_FIFO_KHR;
        for (const VkPresentModeKHR& AvailablePresentMode : PresentModes)
        {
            if (AvailablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                m_PresentMode = AvailablePresentMode;
            }
        }
    }
    else
    {
        LOG("No available presentModes for Swapchain\n");
        return false;
    }

    if (Capabilities.maxImageCount > 0 && m_ImageCount > Capabilities.maxImageCount)
    {
        m_ImageCount = Capabilities.maxImageCount;
    }

    // Create the swapchain
    {
        VkSwapchainCreateInfoKHR SwapChainCreateInfo;
        ZERO_STRUCT(&SwapChainCreateInfo);

        SwapChainCreateInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        SwapChainCreateInfo.surface          = m_Surface;
        SwapChainCreateInfo.minImageCount    = m_ImageCount;
        SwapChainCreateInfo.imageFormat      = m_SwapchainFormat.format;
        SwapChainCreateInfo.imageColorSpace  = m_SwapchainFormat.colorSpace;
        SwapChainCreateInfo.imageExtent      = m_Extent;
        SwapChainCreateInfo.imageArrayLayers = 1;
        SwapChainCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
        SwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        SwapChainCreateInfo.preTransform     = Capabilities.currentTransform;
        SwapChainCreateInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        SwapChainCreateInfo.presentMode      = m_PresentMode;
        SwapChainCreateInfo.clipped          = VK_TRUE;
        SwapChainCreateInfo.oldSwapchain     = VK_NULL_HANDLE;

        VkResult result = vkCreateSwapchainKHR(GetDevice()->GetDevice(), &SwapChainCreateInfo, nullptr, &m_Swapchain);
        if (result != VK_SUCCESS)
        {
            LOG("vkCreateSwapchainKHR failed. Error: %d\n", result);
            return false;
        }
    }

    // Get the images and create ImageViews
    uint32_t RealImageCount = 0;
    vkGetSwapchainImagesKHR(GetDevice()->GetDevice(), m_Swapchain, &RealImageCount, nullptr);
    if (RealImageCount < m_ImageCount)
    {
        LOG("WARNING: Less images than requested in swapchain\n");
    }

    std::vector<VkImage> Images(RealImageCount);
    vkGetSwapchainImagesKHR(GetDevice()->GetDevice(), m_Swapchain, &RealImageCount, Images.data());

    // Create ImageViews for the BackBuffers
    {
        VkImageViewCreateInfo ImageViewCreateInfo;
        ZERO_STRUCT(&ImageViewCreateInfo);

        ImageViewCreateInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ImageViewCreateInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        ImageViewCreateInfo.format                          = m_SwapchainFormat.format;
        ImageViewCreateInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        ImageViewCreateInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        ImageViewCreateInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        ImageViewCreateInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        ImageViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        ImageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
        ImageViewCreateInfo.subresourceRange.levelCount     = 1;
        ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        ImageViewCreateInfo.subresourceRange.layerCount     = 1;

        for (uint32_t i = 0; i < m_ImageCount; i++)
        {
            VkImageView ImageView = VK_NULL_HANDLE;
            ImageViewCreateInfo.image = Images[i];

            VkResult Result = vkCreateImageView(GetDevice()->GetDevice(), &ImageViewCreateInfo, nullptr, &ImageView);
            if (Result != VK_SUCCESS)
            {
                LOG("vkCreateImageView failed. Error: %d\n", Result);
            }
            else
            {
                LOG("Created ImageView\n");
            }

            m_FrameData[i].BackBuffer     = Images[i];
            m_FrameData[i].BackBufferView = ImageView;
        }
    }


    // Acquire the first image
    {
        VkResult Result = AquireNextImage();
        if (Result != VK_SUCCESS)
        {
            LOG("AquireNextImage failed. Error: %d\n", Result);
        }
    }

    return true;
}

bool FSwapchain::CreateSemaphores()
{
    // Setup semaphore structure
    VkSemaphoreCreateInfo SemaphoreCreateInfo = {};
    SemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    SemaphoreCreateInfo.pNext = nullptr;
    SemaphoreCreateInfo.flags = 0;

    // Create semaphores
    m_SemaphoreCount = m_ImageCount + 1;
    m_SemaphoreData.resize(m_SemaphoreCount);
    for (uint32_t i = 0; i < m_SemaphoreCount; i++)
    {
        VkSemaphore ImageSemaphore  = VK_NULL_HANDLE;
        VkSemaphore RenderSemaphore = VK_NULL_HANDLE;

        if (vkCreateSemaphore(GetDevice()->GetDevice(), &SemaphoreCreateInfo, nullptr, &ImageSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(GetDevice()->GetDevice(), &SemaphoreCreateInfo, nullptr, &RenderSemaphore) != VK_SUCCESS)
        {
            LOG("vkCreateSemaphore failed\n");
            return false;
        }
        else
        {
            SetDebugName(GetDevice()->GetDevice(), "ImageSemaphore[" + std::to_string(i) + "]", (uint64_t)ImageSemaphore, VK_OBJECT_TYPE_SEMAPHORE);
            SetDebugName(GetDevice()->GetDevice(), "RenderSemaphore[" + std::to_string(i) + "]", (uint64_t)RenderSemaphore, VK_OBJECT_TYPE_SEMAPHORE);
        }

        m_SemaphoreData[i].ImageSemaphore  = ImageSemaphore;
        m_SemaphoreData[i].RenderSemaphore = RenderSemaphore;
    }

    return true;
}

VkResult FSwapchain::AquireNextImage()
{
    m_SemaphoreIndex = (m_SemaphoreIndex + 1) % m_SemaphoreCount;
    VkSemaphore SignalSemaphore = GetImageSemaphore();
    return vkAcquireNextImageKHR(GetDevice()->GetDevice(), m_Swapchain, UINT64_MAX, SignalSemaphore, VK_NULL_HANDLE, &m_CurrentBufferIndex);
}

void FSwapchain::WaitForImage()
{
    VkSemaphore WaitSemaphores[] = { GetImageSemaphore() };
    VkPipelineStageFlags WaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo SubmitInfo;
    ZERO_STRUCT(&SubmitInfo);

    SubmitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.waitSemaphoreCount = 1;
    SubmitInfo.pWaitSemaphores    = WaitSemaphores;
    SubmitInfo.pWaitDstStageMask  = WaitStages;

    VkResult Result = vkQueueSubmit(GetDevice()->GetGraphicsQueue(), 1, &SubmitInfo, VK_NULL_HANDLE);
    if (Result != VK_SUCCESS)
    {
        LOG("vkQueueSubmit failed. Error: %d\n", Result);
    }
}

void FSwapchain::RecreateSwapchain()
{
    GetDevice()->WaitForIdle();

    ReleaseSwapchainResources();
    CreateSwapchain();
}

void FSwapchain::Resize(uint32_t Width, uint32_t Height)
{
    if (m_Extent.width != Width || m_Extent.height != Height)
    {
        // Since we always acquire an image, we need to wait for it
        WaitForImage();
        GetDevice()->WaitForIdle();

        ReleaseSwapchainResources();
        CreateSwapchain();
        
        LOG("Resized Swapchain: w=%u, h=%u\n", m_Extent.width, m_Extent.height);
    }
}

VkResult FSwapchain::Present()
{
    VkSemaphore WaitSemaphores[] = { GetRenderSemaphore() };

    VkPresentInfoKHR PresentInfo;
    ZERO_STRUCT(&PresentInfo);

    PresentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    PresentInfo.waitSemaphoreCount = 1;
    PresentInfo.pWaitSemaphores    = WaitSemaphores;
    PresentInfo.swapchainCount     = 1;
    PresentInfo.pSwapchains        = &m_Swapchain;
    PresentInfo.pImageIndices      = &m_CurrentBufferIndex;
    PresentInfo.pResults           = nullptr;

    VkResult Result = vkQueuePresentKHR(GetDevice()->GetPresentQueue(), &PresentInfo);
    if (Result == VK_SUCCESS)
    {
        // Acquire next image
        Result = AquireNextImage();
    }

    // if presentation and acquire image failed
    if (Result != VK_SUCCESS)
    {
        if (Result == VK_SUBOPTIMAL_KHR || Result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RecreateSwapchain();
            LOG("Suboptimal or Out Of Date Swapchain\n");
        }
        else
        {
            LOG("Present Failed. Error: %d\n", Result);
        }
    }

    return Result;
}

void FSwapchain::ReleaseSwapchainResources()
{
    // Release BackBuffers
    for (FFrameData& FrameData : m_FrameData)
    {
        FrameData.BackBuffer = VK_NULL_HANDLE;
        if (FrameData.BackBufferView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(GetDevice()->GetDevice(), FrameData.BackBufferView, nullptr);
            FrameData.BackBufferView = VK_NULL_HANDLE;
        }
    }

    // Destroy swapchain
    if (m_Swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(GetDevice()->GetDevice(), m_Swapchain, nullptr);
        m_Swapchain = VK_NULL_HANDLE;
    }
}
