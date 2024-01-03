#include "SwapChain.h"
#include "VulkanContext.h"
#include "VulkanHelper.h"

#define NUM_BACK_BUFFERS (3)

SwapChain* SwapChain::Create(VulkanContext* pContext, GLFWwindow* pWindow)
{
    SwapChain* pSwapChain = new SwapChain(pContext, pWindow);
    if (!pSwapChain->CreateSurface())
    {
        return nullptr;
    }
    
    if (pSwapChain->CreateSemaphores())
    {
        std::cout << "Created Semphores and Fences\n";
    }
    else
    {
        return nullptr;
    }

    if (pSwapChain->CreateSwapChain())
    {
        std::cout << "Created SwapChain\n";
    }
    else
    {
        return nullptr;
    }
    
    return pSwapChain;
}

SwapChain::SwapChain(VulkanContext* pContext, GLFWwindow* pWindow)
    : m_pContext(pContext)
    , m_pWindow(pWindow)
    , m_Surface(VK_NULL_HANDLE)
    , m_Extent{ 0, 0 }
    , m_ImageCount(NUM_BACK_BUFFERS)
    , m_FrameData()
{
    m_FrameData.resize(m_ImageCount);
}

SwapChain::~SwapChain()
{
    for (FrameData& frame : m_FrameData)
    {
        if (frame.ImageSemaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(m_pContext->GetDevice(), frame.ImageSemaphore, nullptr);
            frame.ImageSemaphore = VK_NULL_HANDLE;
        }
        
        if (frame.RenderSemaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(m_pContext->GetDevice(), frame.RenderSemaphore, nullptr);
            frame.RenderSemaphore = VK_NULL_HANDLE;
        }
    }
    
    ReleaseSwapChainResources();
    
    if (m_Surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(m_pContext->GetInstance(), m_Surface, nullptr);
        m_Surface = VK_NULL_HANDLE;
    }
}

bool SwapChain::CreateSurface()
{
    VkResult result = glfwCreateWindowSurface(m_pContext->GetInstance(), m_pWindow, nullptr, &m_Surface);
    if (result != VK_SUCCESS)
    {
        std::cout << "glfwCreateWindowSurface failed  with error: " << result << std::endl;
        return false;
    }

    return true;
}

bool SwapChain::CreateSwapChain()
{
    int32_t width  = 0;
    int32_t height = 0;
    glfwGetFramebufferSize(m_pWindow, &width, &height);
    
    // Get capabilities and formats that are supported
    VkSurfaceCapabilitiesKHR capabilities = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_pContext->GetPhysicalDevice(), m_Surface, &capabilities);
    if (capabilities.currentExtent.width != UINT32_MAX && capabilities.currentExtent.height != UINT32_MAX)
    {
        m_Extent = capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtent = { uint32_t(width), uint32_t(height) };
        actualExtent.width  = std::max(capabilities.minImageExtent.width,  std::min(capabilities.maxImageExtent.width,  actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        m_Extent = actualExtent;
    }
    

    std::vector<VkPresentModeKHR>   presentModes;
    std::vector<VkSurfaceFormatKHR> formats;

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_pContext->GetPhysicalDevice(), m_Surface, &formatCount, nullptr);
    if (formatCount > 0)
    {
        formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_pContext->GetPhysicalDevice(), m_Surface, &formatCount, formats.data());

        for (const auto& availableFormat : formats)
        {
            if (availableFormat.format == VK_FORMAT_R16G16B16A16_SFLOAT && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                m_SwapChainFormat = availableFormat;
            }
            else if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                m_SwapChainFormat = availableFormat;
            }
        }
    }
    else
    {
        std::cout << "No available formats for SwapChain" << std::endl;
        return false;
    }

    
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_pContext->GetPhysicalDevice(), m_Surface, &presentModeCount, nullptr);
    if (presentModeCount > 0)
    {
        presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_pContext->GetPhysicalDevice(), m_Surface, &presentModeCount, presentModes.data());

        // Choose mailbox if available otherwise fifo
        m_PresentMode = VK_PRESENT_MODE_FIFO_KHR;
        for (const VkPresentModeKHR& availablePresentMode : presentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                m_PresentMode = availablePresentMode;
            }
        }
    }
    else
    {
        std::cout << "No available presentModes for SwapChain" << std::endl;
        return false;
    }

    if (capabilities.maxImageCount > 0 && m_ImageCount > capabilities.maxImageCount)
    {
        m_ImageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo;
    ZERO_STRUCT(&createInfo);
    
    createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface          = m_Surface;
    createInfo.minImageCount    = m_ImageCount;
    createInfo.imageFormat      = m_SwapChainFormat.format;
    createInfo.imageColorSpace  = m_SwapChainFormat.colorSpace;
    createInfo.imageExtent      = m_Extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform     = capabilities.currentTransform;
    createInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode      = m_PresentMode;
    createInfo.clipped          = VK_TRUE;
    createInfo.oldSwapchain     = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(m_pContext->GetDevice(), &createInfo, nullptr, &m_SwapChain);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkCreateSwapchainKHR failed. Error: " << result << std::endl;
        return false;
    }

    //Get the images and create imageviews
    uint32_t realImageCount = 0;
    vkGetSwapchainImagesKHR(m_pContext->GetDevice(), m_SwapChain, &realImageCount, nullptr);
    if (realImageCount < m_ImageCount)
    {
        std::cout << "WARNING: Less images than requested in swapchain" << std::endl;
    }

    std::vector<VkImage> images(realImageCount);
    vkGetSwapchainImagesKHR(m_pContext->GetDevice(), m_SwapChain, &realImageCount, images.data());

    VkImageViewCreateInfo imageViewCreateInfo;
    ZERO_STRUCT(&imageViewCreateInfo);
    
    imageViewCreateInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format                          = m_SwapChainFormat.format;
    imageViewCreateInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
    imageViewCreateInfo.subresourceRange.levelCount     = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount     = 1;

    for (uint32_t i = 0; i < m_ImageCount; i++)
    {
        VkImageView imageView = VK_NULL_HANDLE;
        imageViewCreateInfo.image = images[i];

        result = vkCreateImageView(m_pContext->GetDevice(), &imageViewCreateInfo, nullptr, &imageView);
        if (result != VK_SUCCESS)
        {
            std::cout << "vkCreateImageView failed. Error: " << result << std::endl;
        }
        else
        {
            std::cout << "Created ImageView" << std::endl;
        }

        m_FrameData[i].BackBuffer     = images[i];
        m_FrameData[i].BackBufferView = imageView;
    }

    result = AquireNextImage();
    if (result != VK_SUCCESS)
    {
        std::cout << "AquireNextImage failed. Error: " << result << std::endl;
    }

    return true;
}

bool SwapChain::CreateSemaphores()
{
    // Setup semaphore structure
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreInfo.pNext = nullptr;
    semaphoreInfo.flags = 0;

    // Create semaphores
    for (uint32_t i = 0; i < m_ImageCount; i++)
    {
        VkSemaphore imageSemaphore  = VK_NULL_HANDLE;
        VkSemaphore renderSemaphore = VK_NULL_HANDLE;

        if (vkCreateSemaphore(m_pContext->GetDevice(), &semaphoreInfo, nullptr, &imageSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(m_pContext->GetDevice(), &semaphoreInfo, nullptr, &renderSemaphore) != VK_SUCCESS)
        {
            std::cout << "vkCreateSemaphore failed" << std::endl;
            return false;
        }
        else
        {
            SetDebugName(m_pContext->GetDevice(), "ImageSemaphore[" + std::to_string(i) + "]", (uint64_t)imageSemaphore, VK_OBJECT_TYPE_SEMAPHORE);
            SetDebugName(m_pContext->GetDevice(), "RenderSemaphore[" + std::to_string(i) + "]", (uint64_t)renderSemaphore, VK_OBJECT_TYPE_SEMAPHORE);
        }

        m_FrameData[i].ImageSemaphore  = imageSemaphore;
        m_FrameData[i].RenderSemaphore = renderSemaphore;
    }

    return true;
}

VkResult SwapChain::AquireNextImage()
{
    // std::cout << "AquireNextImage" << std::endl;

    VkSemaphore signalSemaphore = m_FrameData[m_SemaphoreIndex].ImageSemaphore;
    return vkAcquireNextImageKHR(m_pContext->GetDevice(), m_SwapChain, UINT64_MAX, signalSemaphore, VK_NULL_HANDLE, &m_CurrentBufferIndex);
}

void SwapChain::RecreateSwapChain()
{
    m_pContext->WaitForIdle();

    ReleaseSwapChainResources();
    CreateSwapChain();
}

void SwapChain::Resize(uint32_t width, uint32_t height)
{
    if (m_Extent.width != width || m_Extent.height != height)
    {
        std::cout << "Resize" << std::endl;
        
        ReleaseSwapChainResources();
        CreateSwapChain();
        
        std::cout << "Resized Buffers: w=" << m_Extent.width << ", h=" << m_Extent.height << std::endl;
    }
}

void SwapChain::Present()
{
    VkSemaphore waitSemaphores[] = { m_FrameData[m_SemaphoreIndex].RenderSemaphore };

    VkPresentInfoKHR presentInfo;
    ZERO_STRUCT(&presentInfo);
    
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = waitSemaphores;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &m_SwapChain;
    presentInfo.pImageIndices      = &m_CurrentBufferIndex;
    presentInfo.pResults           = nullptr;

    VkResult result = vkQueuePresentKHR(m_pContext->GetPresentQueue(), &presentInfo);
    if (result == VK_SUCCESS)
    {
        // Aquire next image
        m_SemaphoreIndex = (m_SemaphoreIndex + 1) % m_ImageCount;
        result = AquireNextImage();
    }

    // if presentation and aquire image failed
    if (result != VK_SUCCESS)
    {
        if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RecreateSwapChain();
            std::cout << "Suboptimal or Out Of Date SwapChain. Result: " << result << std::endl;
        }
        else
        {
            std::cout << "Present Failed. Error: " << result << std::endl;
        }
    }
}

void SwapChain::ReleaseSwapChainResources()
{
    // Release backbuffers
    for (FrameData& frame : m_FrameData)
    {
        frame.BackBuffer = VK_NULL_HANDLE;
        if (frame.BackBufferView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_pContext->GetDevice(), frame.BackBufferView, nullptr);
            frame.BackBufferView = VK_NULL_HANDLE;
        }
    }

    // Destroy swapchain
    if (m_SwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_pContext->GetDevice(), m_SwapChain, nullptr);
        m_SwapChain = VK_NULL_HANDLE;
    }
}
