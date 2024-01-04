#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>
#include <vector>

class Device;

class Swapchain
{
    struct FrameData
    {
        VkImage     BackBuffer      = VK_NULL_HANDLE;
        VkImageView BackBufferView  = VK_NULL_HANDLE;
        VkSemaphore ImageSemaphore  = VK_NULL_HANDLE;
        VkSemaphore RenderSemaphore = VK_NULL_HANDLE;
    };
    
public:
    static Swapchain* Create(Device* pDevice, GLFWwindow* pWindow);
    
    Swapchain(Device* pDevice, GLFWwindow* pWindow);
    ~Swapchain();

    void Resize(uint32_t width, uint32_t height);

    VkResult Present();

    VkSemaphore GetImageSemaphore() const
    {
        return m_FrameData[m_SemaphoreIndex].ImageSemaphore;
    }
    
    VkSemaphore GetRenderSemaphore() const
    {
        return m_FrameData[m_SemaphoreIndex].RenderSemaphore;
    }
    
    VkImage GetImage(uint32_t index) const
    {
        return m_FrameData[index].BackBuffer;
    }
    
    VkImageView GetImageView(uint32_t index) const
    {
        return m_FrameData[index].BackBufferView;
    }
    
    VkFormat GetFormat() const
    {
        return m_SwapchainFormat.format;
    }
    
    VkExtent2D GetExtent() const
    {
        return m_Extent;
    }
    
    uint32_t GetCurrentBackBufferIndex() const
    {
        return m_CurrentBufferIndex;
    }
    
    uint32_t GetNumBackBuffers() const
    {
        return m_ImageCount;
    }
    
private:
    bool CreateSurface();
    bool CreateSemaphores();
    bool CreateSwapchain();
   
    void ReleaseSwapchainResources();
    void RecreateSwapchain();
    
    VkResult AquireNextImage();
    void     WaitForImage();

    Device*                m_pDevice;
    GLFWwindow*            m_pWindow;
    VkSurfaceKHR           m_Surface;
    VkSwapchainKHR         m_Swapchain;
    VkExtent2D             m_Extent;
    VkSurfaceFormatKHR     m_SwapchainFormat;
    VkPresentModeKHR       m_PresentMode;
    uint32_t               m_ImageCount;
    std::vector<FrameData> m_FrameData;
    mutable uint32_t       m_SemaphoreIndex;
    mutable uint32_t       m_CurrentBufferIndex;
};
