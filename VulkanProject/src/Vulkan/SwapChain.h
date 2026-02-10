#pragma once
#include "DeviceChild.h"
#include <vulkan/vulkan.h>
#include <vector>

class CDevice;

class CSwapchain : public CDeviceChild
{
    struct SFrameData
    {
        VkImage     BackBuffer      = VK_NULL_HANDLE;
        VkImageView BackBufferView  = VK_NULL_HANDLE;
    };
    
    struct SSemaphores
    {
        VkSemaphore ImageSemaphore  = VK_NULL_HANDLE;
        VkSemaphore RenderSemaphore = VK_NULL_HANDLE;
    };
    
public:
    static CSwapchain* Create(CDevice* pDevice, GLFWwindow* pWindow);
    
    CSwapchain(CDevice* pDevice, GLFWwindow* pWindow);
    ~CSwapchain();

    void Resize(uint32_t Width, uint32_t Height);
    VkResult Present();

    VkSemaphore GetImageSemaphore() const
    {
        return m_SemaphoreData[m_SemaphoreIndex].ImageSemaphore;
    }
    
    VkSemaphore GetRenderSemaphore() const
    {
        return m_SemaphoreData[m_SemaphoreIndex].RenderSemaphore;
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
    void WaitForImage();

    GLFWwindow*              m_pWindow;
    VkSurfaceKHR             m_Surface;
    VkSwapchainKHR           m_Swapchain;
    VkExtent2D               m_Extent;
    VkSurfaceFormatKHR       m_SwapchainFormat;
    VkPresentModeKHR         m_PresentMode;
    uint32_t                 m_ImageCount;
    uint32_t                 m_SemaphoreCount;
    std::vector<SFrameData>  m_FrameData;
    std::vector<SSemaphores> m_SemaphoreData;
    mutable uint32_t         m_SemaphoreIndex;
    mutable uint32_t         m_CurrentBufferIndex;
};
