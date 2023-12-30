#pragma once
#include "Core.h"
#include "CommandBuffer.h"

#define FRAME_COUNT 3

struct DeviceParams
{
    GLFWwindow* pWindow = nullptr;
    bool bEnableRayTracing;
    bool bEnableValidation;
    bool bVerbose = false;
};

struct QueueFamilyIndices
{
    uint32_t Graphics     = UINT32_MAX;
    uint32_t Presentation = UINT32_MAX;
    uint32_t Compute      = UINT32_MAX;
    uint32_t Transfer     = UINT32_MAX;

    bool IsValid() const
    {
        return Compute != UINT32_MAX && Presentation != UINT32_MAX && Graphics != UINT32_MAX && Transfer != UINT32_MAX;
    }
};

class VulkanContext
{
    struct FrameData
    {
        VkImage     BackBuffer      = VK_NULL_HANDLE;
        VkImageView BackBufferView  = VK_NULL_HANDLE;
        VkSemaphore ImageSemaphore  = VK_NULL_HANDLE;
        VkSemaphore RenderSemaphore = VK_NULL_HANDLE;
    };
    
public:
    static VulkanContext* Create(const DeviceParams& params);

    uint32_t GetQueueFamilyIndex(ECommandQueueType Type);
    
    void ExecuteGraphics(CommandBuffer* pCommandBuffer, VkPipelineStageFlags* pWaitStages);

    void ResizeBuffers(uint32_t width, uint32_t height);
    void WaitForIdle();
    void Present();
    void Destroy();
    
    VkImage GetSwapChainImage(uint32_t index) const
    {
        return m_FrameData[index].BackBuffer;
    }
    
    VkImageView GetSwapChainImageView(uint32_t index) const
    {
        return m_FrameData[index].BackBufferView;
    }
    
    VkFormat GetSwapChainFormat() const
    {
        return m_SwapChainFormat.format;
    }
    
    VkExtent2D GetFramebufferExtent() const
    {
        return m_Extent;
    }
    
    uint32_t GetCurrentBackBufferIndex() const
    {
        return m_CurrentBufferIndex;
    }
    
    uint32_t GetNumBackBuffers() const
    {
        return m_FrameCount;
    }
    
    VkDevice GetDevice() const
    {
        return m_Device;
    }
    
    VkPhysicalDevice GetPhysicalDevice() const
    {
        return m_PhysicalDevice;
    }

    
private:
    VulkanContext();
    ~VulkanContext();

    bool Init(const DeviceParams& props);
    bool CreateInstance(const DeviceParams& props);
    bool CreateDebugMessenger();
    bool CreateSurface(GLFWwindow* pWindow);
    bool CreateDeviceAndQueues(const DeviceParams& props);
    void InitFrameData(uint32_t numFrames);
    bool CreateSemaphores();
    bool CreateSwapChain(uint32_t width, uint32_t height);
    bool QueryPhysicalDevice(const DeviceParams& props);
   
    void ReleaseSwapChainResources();
    void RecreateSwapChain();
    VkResult AquireNextImage();

    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    QueueFamilyIndices GetQueueFamilyIndices(VkPhysicalDevice physicalDevice);
    std::vector<const char*> GetRequiredDeviceExtensions();
    
private:
    VkInstance               m_Instance;
    VkDebugUtilsMessengerEXT m_DebugMessenger;
    VkPhysicalDevice         m_PhysicalDevice;
    VkDevice                 m_Device;
    VkQueue                  m_GraphicsQueue;
    VkQueue                  m_ComputeQueue;
    VkQueue                  m_TransferQueue;
    VkQueue                  m_PresentationQueue;
    VkSurfaceKHR             m_Surface;
    VkSwapchainKHR           m_SwapChain;
    VkSurfaceFormatKHR       m_SwapChainFormat;
    VkExtent2D               m_Extent;
    VkPresentModeKHR         m_PresentMode;

    VkImage                m_DepthStencilBuffer;
    std::vector<FrameData> m_FrameData;
    uint32_t               m_FrameCount;
    mutable uint32_t       m_SemaphoreIndex;
    mutable uint32_t       m_CurrentBufferIndex;
    
    VkPhysicalDeviceFeatures         m_EnabledDeviceFeatures;
    VkPhysicalDeviceProperties       m_DeviceProperties;
    VkPhysicalDeviceFeatures         m_DeviceFeatures;
    VkPhysicalDeviceMemoryProperties m_DeviceMemoryProperties;
        
    QueueFamilyIndices m_QueueFamilyIndices;
    
    bool m_bValidationEnabled;
    bool m_bRayTracingEnabled;
};
