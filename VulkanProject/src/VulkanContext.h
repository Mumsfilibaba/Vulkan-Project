#pragma once
#include "Core.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>

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

class VulkanContext
{
public:
    DECL_NO_COPY(VulkanContext);
    
    bool IsInstanceExtensionAvailable(const char* pExtensionName);
    bool IsDeviceExtensionAvailable(const char* pExtensionName);

    void Present();
    void Destroy();

    static VulkanContext* Create(const DeviceParams& props);
private:
    VulkanContext();
    ~VulkanContext();

    bool Init(const DeviceParams& props);
    bool CreateInstance(const DeviceParams& props);
    bool CreateDebugMessenger();
    bool CreateSurface(GLFWwindow* pWindow);
    bool CreateDeviceAndQueues(const DeviceParams& props);
    bool CreateSwapChain(uint32 width, uint32 height);
    bool QueryPhysicalDevice();
   
    void DestroyDebugMessenger();

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
    VkSurfaceFormatKHR m_Format;
    VkExtent2D m_Extent;
    VkPresentModeKHR m_PresentMode;

    VkPhysicalDeviceFeatures m_EnabledDeviceFeatures;
    VkPhysicalDeviceProperties m_DeviceProperties;
    VkPhysicalDeviceFeatures m_DeviceFeatures;
    VkPhysicalDeviceMemoryProperties m_DeviceMemoryProperties;
    
    std::vector<VkExtensionProperties> m_AvailableInstanceExtension;
    std::vector<VkExtensionProperties> m_AvailableDeviceExtensions;
    std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;
    
    QueueFamilyIndices m_QueueFamilyIndices;
    
    bool m_bValidationEnabled;
    bool m_bRayTracingEnabled;
};
