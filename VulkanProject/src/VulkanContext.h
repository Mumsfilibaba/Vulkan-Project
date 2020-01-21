#pragma once
#include "Core.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>

struct ContextParams
{
    bool bEnableRayTracing;
    bool bEnableValidation;
};

class VulkanSwapChain;

class VulkanContext
{
public:
    VulkanSwapChain* CreateSwapChain(GLFWwindow* pWindow);
    void DestroySwapChain(VulkanSwapChain** ppSwapChain);
    void Release();

    bool IsInstanceExtensionAvailable(const char* pExtensionName);
    bool IsDeviceExtensionAvailable(const char* pExtensionName);

    static VulkanContext* Create(const ContextParams& props);
private:
    DECL_NO_COPY(VulkanContext);
    
    VulkanContext();
    ~VulkanContext();

    bool Init(const ContextParams& props);
    bool CreateInstance(const ContextParams& props);
    bool CreateDebugMessenger();
    bool QueryPhysicalDevice();
    bool CreateDeviceAndQueues(const ContextParams& props);
    void DestroyDebugMessenger();
    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    uint32 GetQueueFamilyIndex(VkQueueFlagBits queueFlags);
private:
    VkInstance m_Instance;
    VkDebugUtilsMessengerEXT m_DebugMessenger;
    VkPhysicalDevice m_PhysicalDevice;
    VkDevice m_Device;
    VkQueue m_GraphicsQueue;
    VkQueue m_ComputeQueue;
    VkQueue m_TransferQueue;
    
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
