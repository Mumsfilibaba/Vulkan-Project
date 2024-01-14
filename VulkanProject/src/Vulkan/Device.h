#pragma once
#include "Core.h"
#include "CommandBuffer.h"

class FSwapchain;

struct FDeviceParams
{
    GLFWwindow* pWindow           = nullptr;
    bool        bEnableRayTracing = false;
    bool        bEnableValidation = false;
    bool        bVerbose          = false;
};

struct FQueueFamilyIndices
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

class FDevice
{
public:
    static FDevice* Create(const FDeviceParams& params);

    FDevice();
    ~FDevice();

    void ExecuteGraphics(FCommandBuffer* pCommandBuffer, FSwapchain* pSwapchain, VkPipelineStageFlags* pWaitStages);

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

    VkQueue GetGraphicsQueue() const
    {
        return m_GraphicsQueue;
    }

    float GetTimestampPeriod() const
    {
        return m_DeviceProperties.limits.timestampPeriod;
    }
    
private:
    bool Init(const FDeviceParams& props);
    bool CreateInstance(const FDeviceParams& props);
    bool CreateDebugMessenger();
    bool CreateDeviceAndQueues(const FDeviceParams& props);
    bool QueryPhysicalDevice(const FDeviceParams& props);

    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    FQueueFamilyIndices      GetQueueFamilyIndices(VkPhysicalDevice physicalDevice);
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
    FQueueFamilyIndices                    m_QueueFamilyIndices;
          
    bool m_bValidationEnabled : 1;
    bool m_bRayTracingEnabled : 1;
};
