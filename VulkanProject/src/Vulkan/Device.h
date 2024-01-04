#pragma once
#include "Core.h"
#include "CommandBuffer.h"

class Swapchain;

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

class Device
{
public:
    static Device* Create(const DeviceParams& params);

    Device();
    ~Device();

    void ExecuteGraphics(CommandBuffer* pCommandBuffer, Swapchain* pSwapchain, VkPipelineStageFlags* pWaitStages);

    void WaitForIdle();

    void Destroy();

    uint32_t GetQueueFamilyIndex(ECommandQueueType Type);
    
    VkDevice GetDevice() const
    {
        return m_Device;
    }
    
    VkPhysicalDevice GetPhysicalDevice() const
    {
        return m_PhysicalDevice;
    }
    
    VkInstance GetInstance() const
    {
        return m_Instance;
    }
    
    VkQueue GetPresentQueue() const
    {
        return m_PresentationQueue;
    }

    float GetTimestampPeriod() const
    {
        return m_DeviceProperties.limits.timestampPeriod;
    }
    
private:
    bool Init(const DeviceParams& props);
    bool CreateInstance(const DeviceParams& props);
    bool CreateDebugMessenger();
    bool CreateDeviceAndQueues(const DeviceParams& props);
    bool QueryPhysicalDevice(const DeviceParams& props);

    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    QueueFamilyIndices       GetQueueFamilyIndices(VkPhysicalDevice physicalDevice);
    std::vector<const char*> GetRequiredDeviceExtensions();
    
    VkInstance               m_Instance;
    VkDebugUtilsMessengerEXT m_DebugMessenger;
    VkPhysicalDevice         m_PhysicalDevice;
    VkDevice                 m_Device;
    
    // Queues
    VkQueue m_GraphicsQueue;
    VkQueue m_ComputeQueue;
    VkQueue m_TransferQueue;
    VkQueue m_PresentationQueue;
    
    // Device Features
    VkPhysicalDeviceFeatures2              m_EnabledDeviceFeatures;
    VkPhysicalDeviceProperties             m_DeviceProperties;
    VkPhysicalDeviceFeatures2              m_DeviceFeatures;
    VkPhysicalDeviceHostQueryResetFeatures m_HostQueryFeatures;
    VkPhysicalDeviceMemoryProperties       m_DeviceMemoryProperties;
    QueueFamilyIndices                     m_QueueFamilyIndices;
          
    bool m_bValidationEnabled : 1;
    bool m_bRayTracingEnabled : 1;
};
