#include "Device.h"
#include "Helpers.h"
#include "Buffer.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "CommandBuffer.h"
#include "Extensions.h"
#include "DeviceMemoryAllocator.h"
#include "DescriptorPool.h"
#include "Swapchain.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) 
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        std::cout << "Validation layer: " << pCallbackData->pMessage << "\n";
    }
    
    return VK_FALSE;
}

FDevice* FDevice::Create(const FDeviceParams& params)
{
    FDevice* pDevice = new FDevice();
    return pDevice->Init(params) ? pDevice : nullptr;
}

FDevice::FDevice()
    : m_Instance(VK_NULL_HANDLE)
    , m_DebugMessenger(VK_NULL_HANDLE)
    , m_PhysicalDevice(VK_NULL_HANDLE)
    , m_Device(VK_NULL_HANDLE)
    , m_GraphicsQueue(VK_NULL_HANDLE)
    , m_ComputeQueue(VK_NULL_HANDLE)
    , m_TransferQueue(VK_NULL_HANDLE)
    , m_EnabledDeviceFeatures()
    , m_DeviceProperties()
    , m_DeviceFeatures()
    , m_DeviceMemoryProperties()
    , m_QueueFamilyIndices()
    , m_bValidationEnabled(false)
    , m_bRayTracingEnabled(false)
{
}

FDevice::~FDevice()
{
    if (m_Device)
    {
        vkDestroyDevice(m_Device, nullptr);
        m_Device = VK_NULL_HANDLE;
    }

    if (m_bValidationEnabled)
    {
        if (FExtensions::vkDestroyDebugUtilsMessengerEXT)
        {
            FExtensions::vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
            m_DebugMessenger = nullptr;
        }
    }
    
    if (m_Instance)
    {
        vkDestroyInstance(m_Instance, nullptr);
        m_Instance = VK_NULL_HANDLE;
    }
}

uint32_t FDevice::GetQueueFamilyIndex(ECommandQueueType Type)
{
    switch (Type)
    {
        case ECommandQueueType::Graphics: return m_QueueFamilyIndices.Graphics;
        case ECommandQueueType::Compute:  return m_QueueFamilyIndices.Compute;
        case ECommandQueueType::Transfer: return m_QueueFamilyIndices.Transfer;
        default: return (uint32_t)-1;
    }
}

void FDevice::ExecuteGraphics(FCommandBuffer* pCommandBuffer, FSwapchain* pSwapchain, VkPipelineStageFlags* pWaitStages)
{
    VkSubmitInfo submitInfo;
    ZERO_STRUCT(&submitInfo);

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[1]   = {};
    VkSemaphore signalSemaphores[1] = {};
    if (pSwapchain)
    {
        assert(pWaitStages != nullptr);

        signalSemaphores[0] = pSwapchain->GetRenderSemaphore();
        waitSemaphores[0]   = pSwapchain->GetImageSemaphore();
        
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = signalSemaphores;
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = waitSemaphores;
        submitInfo.pWaitDstStageMask    = pWaitStages;
    }
    else
    {
        submitInfo.waitSemaphoreCount   = 0;
        submitInfo.pWaitSemaphores      = nullptr;
        submitInfo.pWaitDstStageMask    = nullptr;
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.pSignalSemaphores    = nullptr;
    }

    // Calling execute with nullptr CommandBuffer results in waiting for the current semaphore
    // This seems to be the only way of handling this
    VkFence fence = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffers[1] = {};
    if (pCommandBuffer)
    {
        fence = pCommandBuffer->GetFence();

        commandBuffers[0] = pCommandBuffer->GetCommandBuffer();
        submitInfo.pCommandBuffers    = commandBuffers;
        submitInfo.commandBufferCount = 1;
    }
    else
    {
        submitInfo.pCommandBuffers    = nullptr;
        submitInfo.commandBufferCount = 0;
    }

    VkResult result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, fence);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkQueueSubmit failed. Error: " << result << '\n';
    }
}

void FDevice::WaitForIdle()
{
    vkDeviceWaitIdle(m_Device);
}

void FDevice::Destroy()
{
    delete this;
}

bool FDevice::Init(const FDeviceParams& params)
{
    m_bValidationEnabled = params.bEnableValidation;
    if (CreateInstance(params))
    {
        std::cout << "Created Vulkan Instance\n";
    }
    else
    {
        return false;
    }

    if (m_bValidationEnabled)
    {
        if (CreateDebugMessenger())
        {
            std::cout << "Created Debug Messenger\n";
        }
        else
        {
            return false;
        }
    }

    if (QueryPhysicalDevice(params))
    {
        std::cout << "Queried physical device: " << m_DeviceProperties.deviceName << '\n';
    }
    else
    {
        return false;
    }

    if (CreateDeviceAndQueues(params))
    {
        std::cout << "Created Vulkan Device\n";
    }
    else
    {
        return false;
    }

    return true;
}

bool FDevice::CreateInstance(const FDeviceParams& params)
{
    VkApplicationInfo appInfo;
    ZERO_STRUCT(&appInfo);
    
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "PathTracer";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "PathTracer";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_2;

    // Enable GLFW extensions
    std::vector<const char*> instanceExtensions;

    uint32_t requiredInstanceExtensionCount = 0;
    const char** ppRequiredInstanceExtension = glfwGetRequiredInstanceExtensions(&requiredInstanceExtensionCount);
    if (requiredInstanceExtensionCount > 0)
    {
        std::cout << "Required instance extensions:\n";
        for (uint32_t i = 0; i < requiredInstanceExtensionCount; i++)
        {
            std::cout << "   " << ppRequiredInstanceExtension[i] << '\n';
            instanceExtensions.push_back(ppRequiredInstanceExtension[i]);
        }
    }
    else
    {
        return false;
    }

    //Setup instance
    VkInstanceCreateInfo instanceCreateInfo;
    ZERO_STRUCT(&instanceCreateInfo);
    
    instanceCreateInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    // This extension is needed for MoltenVK, but not supported by RenderDoc, so let's not enable it on other platforms
#if PLATFORM_MAC
    instanceCreateInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

    if (m_bValidationEnabled)
    {
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    //Validate extensions
    uint32_t instanceExtensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
    std::vector<VkExtensionProperties> instanceExtensionProperties(instanceExtensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, instanceExtensionProperties.data());

    if (params.bVerbose)
    {
        std::cout << "Available instance extensions:\n";
        for (VkExtensionProperties extension : instanceExtensionProperties)
        {
            std::cout << "   " << extension.extensionName << '\n';
        }
    }

    std::cout << "Enabled instance extensions:\n";
    for (const char* pEnabledExtension : instanceExtensions)
    {
        std::cout << "   " << pEnabledExtension << '\n';

        bool extensionFound = false;
        for (VkExtensionProperties extension : instanceExtensionProperties)
        {
            if (strcmp(extension.extensionName, pEnabledExtension) == 0)
            {
                extensionFound = true;
                break;
            }
        }

        if (!extensionFound)
        {
            std::cout << "Extension '" << pEnabledExtension << "' not present\n";
        }
    }

    instanceCreateInfo.enabledExtensionCount   = (uint32_t)instanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

    //Setup validation layer
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo;
    ZERO_STRUCT(&debugMessengerCreateInfo);
    
    if (m_bValidationEnabled)
    {
        const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
        uint32_t instanceLayerCount;
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
        std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());

        bool validationLayerPresent = false;
        for (VkLayerProperties layer : instanceLayerProperties)
        {
            if (strcmp(layer.layerName, validationLayerName) == 0)
            {
                validationLayerPresent = true;
                break;
            }
        }

        if (validationLayerPresent)
        {
            PopulateDebugMessengerCreateInfo(debugMessengerCreateInfo);
            instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugMessengerCreateInfo;

            instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
            instanceCreateInfo.enabledLayerCount = 1;
        }
        else
        {
            std::cout << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled\n";
        }
    }

    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_Instance);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkCreateInstance failed\n";
        return false;
    }
    
    //Get instance functions
    FExtensions::vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(m_Instance, "vkSetDebugUtilsObjectNameEXT");
    if (!FExtensions::vkSetDebugUtilsObjectNameEXT)
    {
        std::cout << "Failed to retrieve 'vkSetDebugUtilsObjectNameEXT'\n";
    }
    
    FExtensions::vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
    if (!FExtensions::vkCreateDebugUtilsMessengerEXT)
    {
        std::cout << "Failed to retrieve 'vkCreateDebugUtilsMessengerEXT'\n";
    }
    FExtensions::vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
    if (!FExtensions::vkDestroyDebugUtilsMessengerEXT)
    {
        std::cout << "Failed to retrieve 'vkDestroyDebugUtilsMessengerEXT'\n";
    }
    
    return true;
}

bool FDevice::CreateDebugMessenger()
{
    if (FExtensions::vkCreateDebugUtilsMessengerEXT)
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
        PopulateDebugMessengerCreateInfo(createInfo);

        VkResult result = FExtensions::vkCreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger);
        if (result != VK_SUCCESS)
        {
            std::cout << "vkCreateDebugUtilsMessengerEXT failed. Error: " << result << '\n';
        }
        else
        {
            return true;
        }
    }
    
    return false;
}

bool FDevice::CreateDeviceAndQueues(const FDeviceParams& params)
{
    m_QueueFamilyIndices = GetQueueFamilyIndices(m_PhysicalDevice);

    std::cout << "Using following queueFamilyIndices: ";
    std::cout << "Graphics = "     << m_QueueFamilyIndices.Graphics;
    std::cout << ", Presentation=" << m_QueueFamilyIndices.Presentation;
    std::cout << ", Compute="      << m_QueueFamilyIndices.Compute;
    std::cout << ", Transfer="     << m_QueueFamilyIndices.Transfer << '\n';

    if (m_DeviceProperties.limits.timestampComputeAndGraphics)
    {
        std::cout << "    Timestamps Supported\n";
    }
    else
    {
        std::cout << "    Timestamps NOT Supported\n";
    }

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    const float defaultQueuePriority = 0.0f;

    std::set<uint32_t> uniqueQueueFamilies =
    {
        m_QueueFamilyIndices.Graphics,
        m_QueueFamilyIndices.Compute,
        m_QueueFamilyIndices.Presentation,
        m_QueueFamilyIndices.Transfer
    };

    for (int32_t queueFamiliy : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueInfo = {};
        queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.pNext            = nullptr;
        queueInfo.flags            = 0;
        queueInfo.pQueuePriorities = &defaultQueuePriority;
        queueInfo.queueFamilyIndex = queueFamiliy;
        queueInfo.queueCount       = 1;

        queueCreateInfos.push_back(queueInfo);
    }

    // Get device extensions
    uint32_t deviceExtensionCount;
    vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &deviceExtensionCount, nullptr);
    std::vector<VkExtensionProperties> availableDeviceExtension(deviceExtensionCount);
    vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &deviceExtensionCount, availableDeviceExtension.data());

    bool bEnableDeviceSubset = false;
    for (VkExtensionProperties extension : availableDeviceExtension)
    {
        if (strcmp(extension.extensionName, "VK_KHR_portability_subset") == 0)
        {
            bEnableDeviceSubset = true;
        }
    }
    
    if (params.bVerbose)
    {
        std::cout << "Available device extensions:\n";
        for (VkExtensionProperties extension : availableDeviceExtension)
        {
            std::cout << "   " << extension.extensionName << '\n';
        }
    }

    
    // Enable device extensions
    std::vector<const char*> deviceExtensions = GetRequiredDeviceExtensions();
    if (bEnableDeviceSubset)
    {
        deviceExtensions.push_back("VK_KHR_portability_subset");
    }
    
    if (params.bEnableRayTracing)
    {
        deviceExtensions.push_back(VK_NV_RAY_TRACING_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
        m_bRayTracingEnabled = true;
    }

    // Enable wanted features here
    ZERO_STRUCT(&m_EnabledDeviceFeatures);
    m_EnabledDeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    m_EnabledDeviceFeatures.pNext = &m_HostQueryFeatures;

    // Create the logical device
    VkDeviceCreateInfo deviceCreateInfo;
    ZERO_STRUCT(&deviceCreateInfo);
    
    deviceCreateInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext                = &m_EnabledDeviceFeatures;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos    = queueCreateInfos.data();

    // Verify extensions
    if (deviceExtensions.size() > 0)
    {
        std::cout << "Enabled device extensions:\n";
        for (auto it = deviceExtensions.begin(); it != deviceExtensions.end();)
        {
            bool extensionFound = false;
            for (VkExtensionProperties extension : availableDeviceExtension)
            {
                if (strcmp(extension.extensionName, (*it)) == 0)
                {
                    extensionFound = true;
                    break;
                }
            }

            // Warning that extension is not present, and do not try and activate it
            if (!extensionFound)
            {
                std::cout << "WARNING: Extension '" << (*it) << "' not present\n";
                it = deviceExtensions.erase(it);
            }
            else
            {
                std::cout << "   " << (*it) << '\n';
                it++;
            }
        }

        deviceCreateInfo.enabledExtensionCount   = (uint32_t)deviceExtensions.size();
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    }

    VkResult result = vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device);
    if (result == VK_SUCCESS)
    {
        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.Graphics, 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.Presentation, 0, &m_PresentationQueue);
        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.Transfer, 0, &m_TransferQueue);
        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.Compute, 0, &m_ComputeQueue);
        return true;
    }
    else
    {
        return false;
    }
}

void FDevice::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    ZERO_STRUCT(&createInfo);
    
    createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = VulkanDebugCallback;
}

bool FDevice::QueryPhysicalDevice(const FDeviceParams& params)
{
    // Enumerate devices
    uint32_t gpuCount = 0;
    VkResult result = vkEnumeratePhysicalDevices(m_Instance, &gpuCount, nullptr);

    std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
    result = vkEnumeratePhysicalDevices(m_Instance, &gpuCount, physicalDevices.data());
    if (result != VK_SUCCESS || gpuCount < 1) 
    {
        std::cerr << "vkEnumeratePhysicalDevices failed. Error: " << result << '\n';
        return false;
    }

    // Start with the first one in case we do not find any suitable
    m_PhysicalDevice = physicalDevices[0];

    // GPU selection
    std::cout << "Available GPUs\n";
    for (VkPhysicalDevice physicalDevice : physicalDevices)
    {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
        
        std::cout << "   " << physicalDeviceProperties.deviceName << '\n';

        VkPhysicalDeviceFeatures physicalDeviceFeatures;
        vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

        // Check for adapter features
        if (!physicalDeviceFeatures.samplerAnisotropy)
        {
            std::cout << "Anisotropic filtering is not supported by adapter\n";
            continue;
        }

        // Find indices for queue-families
        FQueueFamilyIndices indices = GetQueueFamilyIndices(physicalDevice);
        if (!indices.IsValid())
        {
            std::cout << "Failed to find a suitable queue-families\n";
            return false;
        }

        // Check if required extension for device is supported
        std::vector<const char*> deviceExtensions = GetRequiredDeviceExtensions();

        uint32_t deviceExtensionCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr);
        std::vector<VkExtensionProperties> availableDeviceExtension(deviceExtensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, availableDeviceExtension.data());
        
        if (params.bVerbose)
        {
            std::cout << "      Available extensions:\n";
            for (const auto& extension : availableDeviceExtension)
            {
                std::cout << "         " << extension.extensionName << '\n';
            }
        }
        
        bool extensionsFound = false;
        for (const auto& extensionName : deviceExtensions)
        {
            extensionsFound = false;
            for (const auto& extension : availableDeviceExtension)
            {
                if (strcmp(extension.extensionName, extensionName) == 0)
                {
                    extensionsFound = true;
                    break;
                }
            }
            
            if (!extensionsFound)
            {
                std::cout << extensionName << " is not supported\n";
            }
        }

        if (extensionsFound)
        {
            // If we came this far we have found a suitable adapter
            m_PhysicalDevice = physicalDevice;
            break;
        }
        else
        {
            std::cout << "Some extensions were not supported on '" << physicalDeviceProperties.deviceName << "'\n";
        }
    }
    
    
    ZERO_STRUCT(&m_HostQueryFeatures);
    m_HostQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES;
    
    ZERO_STRUCT(&m_DeviceFeatures);
    m_DeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    m_DeviceFeatures.pNext = &m_HostQueryFeatures;

    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_DeviceProperties);
    vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &m_DeviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_DeviceMemoryProperties);
    return true;
}

// Helper function
static uint32_t GetQueueFamilyIndex(VkQueueFlagBits queueFlags, const std::vector<VkQueueFamilyProperties>& queueFamilies)
{
    if (queueFlags & VK_QUEUE_COMPUTE_BIT)
    {
        for (uint32_t i = 0; i < uint32_t(queueFamilies.size()); i++)
        {
            if ((queueFamilies[i].queueFlags & queueFlags) && ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
            {
                return i;
            }
        }
    }

    if (queueFlags & VK_QUEUE_TRANSFER_BIT)
    {
        for (uint32_t i = 0; i < uint32_t(queueFamilies.size()); i++)
        {
            if ((queueFamilies[i].queueFlags & queueFlags) && ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
            {
                return i;
            }
        }
    }

    for (uint32_t i = 0; i < uint32_t(queueFamilies.size()); i++)
    {
        if (queueFamilies[i].queueFlags & queueFlags)
        {
            return i;
        }
    }

    return UINT32_MAX;
}

FQueueFamilyIndices FDevice::GetQueueFamilyIndices(VkPhysicalDevice physicalDevice)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    FQueueFamilyIndices indices = {};
    indices.Compute  = ::GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT, queueFamilies);
    indices.Transfer = ::GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT, queueFamilies);
    indices.Graphics = ::GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT, queueFamilies);
    
    // TODO: Do not just assume that graphics support presentation, check this
    indices.Presentation = indices.Graphics;
    return indices;
}

std::vector<const char*> FDevice::GetRequiredDeviceExtensions()
{
    std::vector<const char*> deviceExtensions;
    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    return deviceExtensions;
}
