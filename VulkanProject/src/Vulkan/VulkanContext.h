#pragma once
#include "Core.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

#define FRAME_COUNT 3

struct DeviceParams
{
    GLFWwindow* pWindow;
    bool bEnableRayTracing;
    bool bEnableValidation;
};

struct QueueFamilyIndices
{
    uint32 Graphics     = UINT32_MAX;
    uint32 Presentation = UINT32_MAX;
    uint32 Compute      = UINT32_MAX;
    uint32 Transfer     = UINT32_MAX;

    bool IsValid() const
    {
        return Compute != UINT32_MAX && Presentation != UINT32_MAX && Graphics != UINT32_MAX && Transfer != UINT32_MAX;
    }
};

struct ShaderModuleParams;
struct CommandBufferParams;
struct RenderPassParams;
struct FramebufferParams;
struct GraphicsPipelineStateParams;

class VulkanRenderPass;
class VulkanFramebuffer;
class VulkanShaderModule;
class VulkanCommandBuffer;
class VulkanGraphicsPipelineState;

class VulkanContext
{
private:
    struct FrameData
    {
        VkImage BackBuffer          = VK_NULL_HANDLE;
        VkImageView BackBufferView  = VK_NULL_HANDLE;
        VkSemaphore ImageSemaphore  = VK_NULL_HANDLE;
        VkSemaphore RenderSemaphore = VK_NULL_HANDLE;
    };
public:
    DECL_NO_COPY(VulkanContext);
    
    VulkanRenderPass* CreateRenderPass(const RenderPassParams& params);
    VulkanFramebuffer* CreateFrameBuffer(const FramebufferParams& params);
    VulkanShaderModule* CreateShaderModule(const ShaderModuleParams& params);
    VulkanCommandBuffer* CreateCommandBuffer(const CommandBufferParams& params);
    VulkanGraphicsPipelineState* CreateGraphicsPipelineState(const GraphicsPipelineStateParams& params);

    void ExecuteGraphics(VulkanCommandBuffer* pCommandBuffer, VkPipelineStageFlags* pWaitStages);

    void ResizeBuffers(uint32 width, uint32 height);
    void WaitForIdle();
    void Present();
    void Destroy();

    void SetDebugName(const std::string& name, uint64 vulkanHandle, VkObjectType type);
    
    VkImage GetSwapChainImage(uint32 index) const { return m_FrameData[index].BackBuffer; }
    VkImageView GetSwapChainImageView(uint32 index) const { return m_FrameData[index].BackBufferView; }
    VkFormat GetSwapChainFormat() const { return m_SwapChainFormat.format; }
    VkExtent2D GetFramebufferExtent() const { return m_Extent; }
    uint32 GetCurrentBackBufferIndex() const { return m_CurrentBufferIndex; }
    uint32 GetImageCount() const { return m_FrameCount; }

    static VulkanContext* Create(const DeviceParams& props);
private:
    VulkanContext();
    ~VulkanContext();

    bool Init(const DeviceParams& props);
    bool CreateInstance(const DeviceParams& props);
    bool CreateDebugMessenger();
    bool CreateSurface(GLFWwindow* pWindow);
    bool CreateDeviceAndQueues(const DeviceParams& props);
    void InitFrameData(uint32 numFrames);
    bool CreateSemaphores();
    bool CreateSwapChain(uint32 width, uint32 height);
    bool QueryPhysicalDevice();
   
    void ReleaseSwapChainResources();
    void RecreateSwapChain();
    VkResult AquireNextImage();

    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    QueueFamilyIndices GetQueueFamilyIndices(VkPhysicalDevice physicalDevice);
    std::vector<const char*> GetRequiredDeviceExtensions();
private:
    VkInstance m_Instance;
    VkDebugUtilsMessengerEXT m_DebugMessenger;
    VkPhysicalDevice m_PhysicalDevice;
    VkDevice m_Device;
    VkQueue m_GraphicsQueue;
    VkQueue m_ComputeQueue;
    VkQueue m_TransferQueue;
    VkQueue m_PresentationQueue;
    VkSurfaceKHR m_Surface;
    VkSwapchainKHR m_SwapChain;
    VkSurfaceFormatKHR m_SwapChainFormat;
    VkExtent2D m_Extent;
    VkPresentModeKHR m_PresentMode;

    VkImage m_DepthStencilBuffer;
    std::vector<FrameData> m_FrameData;
    uint32 m_FrameCount;
    mutable uint32 m_SemaphoreIndex;
    mutable uint32 m_CurrentBufferIndex;
    
    VkPhysicalDeviceFeatures m_EnabledDeviceFeatures;
    VkPhysicalDeviceProperties m_DeviceProperties;
    VkPhysicalDeviceFeatures m_DeviceFeatures;
    VkPhysicalDeviceMemoryProperties m_DeviceMemoryProperties;
        
    QueueFamilyIndices m_QueueFamilyIndices;
    
    bool m_bValidationEnabled;
    bool m_bRayTracingEnabled;
    
    static PFN_vkSetDebugUtilsObjectNameEXT    vkSetDebugUtilsObjectNameEXT;
    static PFN_vkCreateDebugUtilsMessengerEXT  vkCreateDebugUtilsMessengerEXT;
    static PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
};
