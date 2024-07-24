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
#include "BindlessManager.h"

#if PLATFORM_MAC
    #include <dlfcn.h>
#endif

#define BREAK_ON_ERROR 1

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
{
    if (MessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        LOG("[Vulkan] %s\n", pCallbackData->pMessage);

    #if BREAK_ON_ERROR
        if (MessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            DEBUG_BREAK();
        }
    #endif
    }
    
    return VK_FALSE;
}

FDevice* FDevice::Create(const FDeviceParams& Params)
{
    FDevice* pDevice = new FDevice();
    return pDevice->Init(Params) ? pDevice : nullptr;
}

FDevice::FDevice()
    : m_Instance(VK_NULL_HANDLE)
    , m_DebugMessenger(VK_NULL_HANDLE)
    , m_PhysicalDevice(VK_NULL_HANDLE)
    , m_Device(VK_NULL_HANDLE)
    , m_pBindlessManager(nullptr)
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
    , m_bBindlessSupported(false)
{
}

FDevice::~FDevice()
{
    SAFE_DELETE(m_pBindlessManager);
    
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
    VkSubmitInfo SubmitInfo;
    ZERO_STRUCT(&SubmitInfo);

    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore WaitSemaphores[1]   = {};
    VkSemaphore SignalSemaphores[1] = {};
    if (pSwapchain)
    {
        assert(pWaitStages != nullptr);

        SignalSemaphores[0] = pSwapchain->GetRenderSemaphore();
        WaitSemaphores[0]   = pSwapchain->GetImageSemaphore();
        
        SubmitInfo.signalSemaphoreCount = 1;
        SubmitInfo.pSignalSemaphores    = SignalSemaphores;
        SubmitInfo.waitSemaphoreCount   = 1;
        SubmitInfo.pWaitSemaphores      = WaitSemaphores;
        SubmitInfo.pWaitDstStageMask    = pWaitStages;
    }
    else
    {
        SubmitInfo.waitSemaphoreCount   = 0;
        SubmitInfo.pWaitSemaphores      = nullptr;
        SubmitInfo.pWaitDstStageMask    = nullptr;
        SubmitInfo.signalSemaphoreCount = 0;
        SubmitInfo.pSignalSemaphores    = nullptr;
    }

    // Calling execute with nullptr CommandBuffer results in waiting for the current semaphore
    VkFence Fence = VK_NULL_HANDLE;
    VkCommandBuffer CommandBuffers[1];
    if (pCommandBuffer)
    {
        CommandBuffers[0] = pCommandBuffer->GetCommandBuffer();
        Fence = pCommandBuffer->GetFence();

        SubmitInfo.pCommandBuffers    = CommandBuffers;
        SubmitInfo.commandBufferCount = 1;
    }
    else
    {
        SubmitInfo.pCommandBuffers    = nullptr;
        SubmitInfo.commandBufferCount = 0;
    }

    VkResult Result = vkQueueSubmit(m_GraphicsQueue, 1, &SubmitInfo, Fence);
    if (Result != VK_SUCCESS)
    {
        LOG("vkQueueSubmit failed. Error: %d\n", Result);
    }
}

void FDevice::WaitForIdle()
{
    VkResult Result = vkDeviceWaitIdle(m_Device);
    if (Result != VK_SUCCESS)
    {
        DEBUG_BREAK();
    }
}

void FDevice::Destroy()
{
    delete this;
}

bool FDevice::Init(const FDeviceParams& Params)
{
    m_bValidationEnabled = Params.bEnableValidation;
    if (CreateInstance(Params))
    {
        LOG("Created Vulkan Instance\n");
    }
    else
    {
        return false;
    }

    if (m_bValidationEnabled)
    {
        if (CreateDebugMessenger())
        {
            LOG("Created Debug Messenger\n");
        }
        else
        {
            return false;
        }
    }

    if (QueryPhysicalDevice(Params))
    {
        LOG("Queried physical device: %s\n", m_DeviceProperties.deviceName);
    }
    else
    {
        return false;
    }

    if (CreateDeviceAndQueues(Params))
    {
        LOG("Created Vulkan Device\n");
    }
    else
    {
        return false;
    }

    // Create BindlessManager
    if (m_bBindlessSupported)
    {
        m_pBindlessManager = FBindlessManager::Create(this);
        if (!m_pBindlessManager)
        {
            return false;
        }
    }
    
    return true;
}

bool FDevice::CreateInstance(const FDeviceParams& Params)
{
    VkApplicationInfo ApplicationInfo;
    ZERO_STRUCT(&ApplicationInfo);
    
    ApplicationInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ApplicationInfo.pApplicationName   = "PathTracer";
    ApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    ApplicationInfo.pEngineName        = "PathTracer";
    ApplicationInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    ApplicationInfo.apiVersion         = VK_API_VERSION_1_2;

    // Enable GLFW extensions
    std::vector<const char*> InstanceExtensions;

    uint32_t RequiredInstanceExtensionCount = 0;
    const char** ppRequiredInstanceExtension = glfwGetRequiredInstanceExtensions(&RequiredInstanceExtensionCount);
    if (RequiredInstanceExtensionCount > 0)
    {
        LOG("Required instance extensions:\n");
        for (uint32_t i = 0; i < RequiredInstanceExtensionCount; i++)
        {
            LOG("   %s\n", ppRequiredInstanceExtension[i]);
            InstanceExtensions.push_back(ppRequiredInstanceExtension[i]);
        }
    }
    else
    {
        return false;
    }

    // Setup instance
    VkInstanceCreateInfo InstanceCreateInfo;
    ZERO_STRUCT(&InstanceCreateInfo);
    
    InstanceCreateInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    InstanceCreateInfo.pApplicationInfo = &ApplicationInfo;

    // This extension is needed for MoltenVK, but not supported by RenderDoc, so let's not enable it on other platforms
#if PLATFORM_MAC
    InstanceCreateInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    InstanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    
    auto libMoltenVK = dlopen("libMoltenVK.dylib", RTLD_LAZY);
    PFN_vkGetMoltenVKConfigurationMVK GetMoltenVKConfigurationMVK = (PFN_vkGetMoltenVKConfigurationMVK)dlsym(libMoltenVK, "vkGetMoltenVKConfigurationMVK");
    assert(GetMoltenVKConfigurationMVK != nullptr);
    PFN_vkSetMoltenVKConfigurationMVK SetMoltenVKConfigurationMVK = (PFN_vkSetMoltenVKConfigurationMVK)dlsym(libMoltenVK, "vkSetMoltenVKConfigurationMVK");
    assert(SetMoltenVKConfigurationMVK != nullptr);

    MVKConfiguration MvkConfig;
    size_t mvkConfigSize = sizeof(MVKConfiguration);
    GetMoltenVKConfigurationMVK(VK_NULL_HANDLE, &MvkConfig, &mvkConfigSize);
    
    MvkConfig.useMetalArgumentBuffers = MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS_ALWAYS;
    SetMoltenVKConfigurationMVK(VK_NULL_HANDLE, &MvkConfig, &mvkConfigSize);
#endif

    if (m_bValidationEnabled)
    {
        InstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Validate extensions
    uint32_t InstanceExtensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &InstanceExtensionCount, nullptr);
    std::vector<VkExtensionProperties> InstanceExtensionProperties(InstanceExtensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &InstanceExtensionCount, InstanceExtensionProperties.data());

    if (Params.bVerbose)
    {
        LOG("Available instance extensions:\n");
        for (VkExtensionProperties Extension : InstanceExtensionProperties)
        {
            LOG("   %s\n", Extension.extensionName);
        }
    }

    LOG("Enabled instance extensions:\n");
    for (const char* pEnabledExtension : InstanceExtensions)
    {
        LOG("   %s\n", pEnabledExtension);

        bool bExtensionFound = false;
        for (VkExtensionProperties Extension : InstanceExtensionProperties)
        {
            if (strcmp(Extension.extensionName, pEnabledExtension) == 0)
            {
                bExtensionFound = true;
                break;
            }
        }

        if (!bExtensionFound)
        {
            LOG("Extension '%s' not present\n", pEnabledExtension);
        }
    }

    InstanceCreateInfo.enabledExtensionCount   = (uint32_t)InstanceExtensions.size();
    InstanceCreateInfo.ppEnabledExtensionNames = InstanceExtensions.data();

    // Setup validation layer
    VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo;
    ZERO_STRUCT(&DebugMessengerCreateInfo);
    
    if (m_bValidationEnabled)
    {
        const char* ValidationLayerName = "VK_LAYER_KHRONOS_validation";
        uint32_t InstanceLayerCount;
        vkEnumerateInstanceLayerProperties(&InstanceLayerCount, nullptr);
        std::vector<VkLayerProperties> InstanceLayerProperties(InstanceLayerCount);
        vkEnumerateInstanceLayerProperties(&InstanceLayerCount, InstanceLayerProperties.data());

        bool bValidationLayerPresent = false;
        for (VkLayerProperties Layer : InstanceLayerProperties)
        {
            if (strcmp(Layer.layerName, ValidationLayerName) == 0)
            {
                bValidationLayerPresent = true;
                break;
            }
        }

        if (bValidationLayerPresent)
        {
            PopulateDebugMessengerCreateInfo(DebugMessengerCreateInfo);
            InstanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&DebugMessengerCreateInfo;

            InstanceCreateInfo.ppEnabledLayerNames = &ValidationLayerName;
            InstanceCreateInfo.enabledLayerCount = 1;
        }
        else
        {
            LOG("Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled\n");
        }
    }

    VkResult Result = vkCreateInstance(&InstanceCreateInfo, nullptr, &m_Instance);
    if (Result != VK_SUCCESS)
    {
        LOG("vkCreateInstance failed\n");
        return false;
    }
    
    // Get instance functions
    FExtensions::vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(m_Instance, "vkSetDebugUtilsObjectNameEXT");
    if (!FExtensions::vkSetDebugUtilsObjectNameEXT)
    {
        LOG("Failed to retrieve 'vkSetDebugUtilsObjectNameEXT'\n");
    }
    
    FExtensions::vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
    if (!FExtensions::vkCreateDebugUtilsMessengerEXT)
    {
        LOG("Failed to retrieve 'vkCreateDebugUtilsMessengerEXT'\n");
    }
    FExtensions::vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
    if (!FExtensions::vkDestroyDebugUtilsMessengerEXT)
    {
        LOG("Failed to retrieve 'vkDestroyDebugUtilsMessengerEXT'\n");
    }
    
    return true;
}

bool FDevice::CreateDebugMessenger()
{
    if (FExtensions::vkCreateDebugUtilsMessengerEXT)
    {
        VkDebugUtilsMessengerCreateInfoEXT CreateInfo = {};
        PopulateDebugMessengerCreateInfo(CreateInfo);

        VkResult Result = FExtensions::vkCreateDebugUtilsMessengerEXT(m_Instance, &CreateInfo, nullptr, &m_DebugMessenger);
        if (Result != VK_SUCCESS)
        {
            LOG("vkCreateDebugUtilsMessengerEXT failed. Error: %d\n", Result);
        }
        else
        {
            return true;
        }
    }
    
    return false;
}

bool FDevice::CreateDeviceAndQueues(const FDeviceParams& Params)
{
    m_QueueFamilyIndices = GetQueueFamilyIndices(m_PhysicalDevice);

    LOG("Using following queueFamilyIndices: Graphics=%d, Presentation=%d, Compute=%d, Transfer=%d\n",
        m_QueueFamilyIndices.Graphics,
        m_QueueFamilyIndices.Presentation,
        m_QueueFamilyIndices.Compute,
        m_QueueFamilyIndices.Transfer);
    
    if (m_DeviceProperties.limits.timestampComputeAndGraphics)
    {
        LOG("    Timestamps Supported\n");
    }
    else
    {
        LOG("    Timestamps NOT Supported\n");
    }
    
    std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
    const float DefaultQueuePriority = 0.0f;
    
    std::set<uint32_t> UniqueQueueFamilies =
    {
        m_QueueFamilyIndices.Graphics,
        m_QueueFamilyIndices.Compute,
        m_QueueFamilyIndices.Presentation,
        m_QueueFamilyIndices.Transfer
    };
    
    for (int32_t QueueFamiliy : UniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo QueueInfo = {};
        QueueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        QueueInfo.pNext            = nullptr;
        QueueInfo.flags            = 0;
        QueueInfo.pQueuePriorities = &DefaultQueuePriority;
        QueueInfo.queueFamilyIndex = QueueFamiliy;
        QueueInfo.queueCount       = 1;
        
        QueueCreateInfos.push_back(QueueInfo);
    }
    
    // Get device extensions
    uint32_t DeviceExtensionCount;
    vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &DeviceExtensionCount, nullptr);
    std::vector<VkExtensionProperties> AvailableDeviceExtension(DeviceExtensionCount);
    vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &DeviceExtensionCount, AvailableDeviceExtension.data());
    
    bool bEnableDeviceSubset = false;
    for (VkExtensionProperties Extension : AvailableDeviceExtension)
    {
        if (strcmp(Extension.extensionName, "VK_KHR_portability_subset") == 0)
        {
            bEnableDeviceSubset = true;
        }
    }
    
    if (Params.bVerbose)
    {
        LOG("Available device extensions:\n");
        
        for (VkExtensionProperties Extension : AvailableDeviceExtension)
        {
            LOG("   %s\n", Extension.extensionName);
        }
    }
    
    // Enable device extensions
    std::vector<const char*> DeviceExtensions = GetRequiredDeviceExtensions();
    if (bEnableDeviceSubset)
    {
        DeviceExtensions.push_back("VK_KHR_portability_subset");
    }
    
    if (Params.bEnableRayTracing)
    {
        DeviceExtensions.push_back(VK_NV_RAY_TRACING_EXTENSION_NAME);
        DeviceExtensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
        m_bRayTracingEnabled = true;
    }
    
    // Enable wanted features here
    ZERO_STRUCT(&m_EnabledDeviceFeatures);
    m_EnabledDeviceFeatures.sType                      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    m_EnabledDeviceFeatures.pNext                      = &m_HostQueryFeatures;
    m_EnabledDeviceFeatures.features.fillModeNonSolid  = VK_TRUE;
    m_EnabledDeviceFeatures.features.samplerAnisotropy = VK_TRUE;
    
    const bool bBindlessSupported =
        m_DescriptorIndexFeatures.descriptorBindingPartiallyBound && m_DescriptorIndexFeatures.runtimeDescriptorArray &&
        m_DescriptorIndexFeatures.descriptorBindingSampledImageUpdateAfterBind && m_DescriptorIndexFeatures.shaderSampledImageArrayNonUniformIndexing &&
        m_DescriptorIndexFeatures.descriptorBindingVariableDescriptorCount;
    
    VkPhysicalDeviceDescriptorIndexingFeatures DescriptorIndexingFeatures;
    if (bBindlessSupported)
    {
        ZERO_STRUCT(&DescriptorIndexingFeatures);
        DescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
        DescriptorIndexingFeatures.pNext = nullptr;
        DescriptorIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
        DescriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
        DescriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
        DescriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        DescriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
        m_HostQueryFeatures.pNext = &DescriptorIndexingFeatures;
    }

    // Create the logical device
    VkDeviceCreateInfo DeviceCreateInfo;
    ZERO_STRUCT(&DeviceCreateInfo);
    
    DeviceCreateInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceCreateInfo.pNext                = &m_EnabledDeviceFeatures;
    DeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(QueueCreateInfos.size());
    DeviceCreateInfo.pQueueCreateInfos    = QueueCreateInfos.data();

    // Verify extensions
    if (DeviceExtensions.size() > 0)
    {
        LOG("Enabled device extensions:\n");
        for (auto it = DeviceExtensions.begin(); it != DeviceExtensions.end();)
        {
            bool bExtensionFound = false;
            for (VkExtensionProperties extension : AvailableDeviceExtension)
            {
                if (strcmp(extension.extensionName, (*it)) == 0)
                {
                    bExtensionFound = true;
                    break;
                }
            }

            // Warning that extension is not present, and do not try and activate it
            if (!bExtensionFound)
            {
                LOG("WARNING: Extension '%s' not present\n", (*it));
                it = DeviceExtensions.erase(it);
            }
            else
            {
                LOG("   %s\n", (*it));
                it++;
            }
        }

        DeviceCreateInfo.enabledExtensionCount   = (uint32_t)DeviceExtensions.size();
        DeviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();
    }

    VkResult Result = vkCreateDevice(m_PhysicalDevice, &DeviceCreateInfo, nullptr, &m_Device);
    if (Result == VK_SUCCESS)
    {
        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.Graphics, 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.Presentation, 0, &m_PresentationQueue);
        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.Transfer, 0, &m_TransferQueue);
        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.Compute, 0, &m_ComputeQueue);

        QueryPhysicalDeviceFeatures();
        m_bBindlessSupported = bBindlessSupported;
        return true;
    }
    else
    {
        return false;
    }
}

void FDevice::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& CreateInfo)
{
    ZERO_STRUCT(&CreateInfo);
    
    CreateInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    CreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    CreateInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    CreateInfo.pfnUserCallback = VulkanDebugCallback;
}

bool FDevice::QueryPhysicalDevice(const FDeviceParams& Params)
{
    // Enumerate devices
    uint32_t GpuCount = 0;
    VkResult Result = vkEnumeratePhysicalDevices(m_Instance, &GpuCount, nullptr);

    std::vector<VkPhysicalDevice> PhysicalDevices(GpuCount);
    Result = vkEnumeratePhysicalDevices(m_Instance, &GpuCount, PhysicalDevices.data());
    if (Result != VK_SUCCESS || GpuCount < 1) 
    {
        LOG("vkEnumeratePhysicalDevices failed. Error: %d\n", Result);
        return false;
    }

    // Start with the first one in case we do not find any suitable
    m_PhysicalDevice = PhysicalDevices[0];

    // GPU selection
    LOG("Available GPUs\n");
    for (VkPhysicalDevice PhysicalDevice : PhysicalDevices)
    {
        VkPhysicalDeviceProperties PhysicalDeviceProperties;
        vkGetPhysicalDeviceProperties(PhysicalDevice, &PhysicalDeviceProperties);
        
        LOG("   %s\n", PhysicalDeviceProperties.deviceName);

        VkPhysicalDeviceFeatures PhysicalDeviceFeatures;
        vkGetPhysicalDeviceFeatures(PhysicalDevice, &PhysicalDeviceFeatures);

        // Check for adapter features
        if (!PhysicalDeviceFeatures.samplerAnisotropy)
        {
            LOG("'SamplerAnisotropy' is not supported by adapter\n");
            continue;
        }
        if (!PhysicalDeviceFeatures.fillModeNonSolid)
        {
            LOG("'FillModeNonSolid' is not supported by adapter\n");
            continue;
        }

        // Find indices for queue-families
        FQueueFamilyIndices Indices = GetQueueFamilyIndices(PhysicalDevice);
        if (!Indices.IsValid())
        {
            LOG("Failed to find a suitable queue-families\n");
            return false;
        }

        // Check if required extension for device is supported
        std::vector<const char*> DeviceExtensions = GetRequiredDeviceExtensions();

        uint32_t DeviceExtensionCount;
        vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &DeviceExtensionCount, nullptr);
        std::vector<VkExtensionProperties> AvailableDeviceExtension(DeviceExtensionCount);
        vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &DeviceExtensionCount, AvailableDeviceExtension.data());
        
        if (Params.bVerbose)
        {
            LOG("      Available extensions:\n");
            for (const auto& Extension : AvailableDeviceExtension)
            {
                LOG("         %s\n", Extension.extensionName);
            }
        }
        
        bool bExtensionsFound = false;
        for (const auto& ExtensionName : DeviceExtensions)
        {
            bExtensionsFound = false;
            for (const auto& Extension : AvailableDeviceExtension)
            {
                if (strcmp(Extension.extensionName, ExtensionName) == 0)
                {
                    bExtensionsFound = true;
                    break;
                }
            }
            
            if (!bExtensionsFound)
            {
                LOG("'%s' is not supported\n", ExtensionName);
            }
        }

        if (bExtensionsFound)
        {
            // If we came this far we have found a suitable adapter
            m_PhysicalDevice = PhysicalDevice;
            break;
        }
        else
        {
            LOG("Some extensions were not supported on '%s'\n", PhysicalDeviceProperties.deviceName);
        }
    }

    QueryPhysicalDeviceFeatures();
    return true;
}

void FDevice::QueryPhysicalDeviceFeatures()
{
    ZERO_STRUCT(&m_DescriptorIndexFeatures);
    m_DescriptorIndexFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;

    ZERO_STRUCT(&m_HostQueryFeatures);
    m_HostQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES;
    m_HostQueryFeatures.pNext = &m_DescriptorIndexFeatures;

    ZERO_STRUCT(&m_DeviceFeatures);
    m_DeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    m_DeviceFeatures.pNext = &m_HostQueryFeatures;

    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_DeviceProperties);
    vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &m_DeviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_DeviceMemoryProperties);
}

// Helper function
static uint32_t GetQueueFamilyIndex(VkQueueFlagBits QueueFlags, const std::vector<VkQueueFamilyProperties>& QueueFamilies)
{
    if (QueueFlags & VK_QUEUE_COMPUTE_BIT)
    {
        for (uint32_t i = 0; i < uint32_t(QueueFamilies.size()); i++)
        {
            if ((QueueFamilies[i].queueFlags & QueueFlags) && ((QueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
            {
                return i;
            }
        }
    }

    if (QueueFlags & VK_QUEUE_TRANSFER_BIT)
    {
        for (uint32_t i = 0; i < uint32_t(QueueFamilies.size()); i++)
        {
            if ((QueueFamilies[i].queueFlags & QueueFlags) && ((QueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((QueueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
            {
                return i;
            }
        }
    }

    for (uint32_t i = 0; i < uint32_t(QueueFamilies.size()); i++)
    {
        if (QueueFamilies[i].queueFlags & QueueFlags)
        {
            return i;
        }
    }

    return UINT32_MAX;
}

FQueueFamilyIndices FDevice::GetQueueFamilyIndices(VkPhysicalDevice PhysicalDevice)
{
    uint32_t QueueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, QueueFamilies.data());

    FQueueFamilyIndices Indices = {};
    Indices.Compute  = ::GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT, QueueFamilies);
    Indices.Transfer = ::GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT, QueueFamilies);
    Indices.Graphics = ::GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT, QueueFamilies);
    
    // TODO: Do not just assume that graphics support presentation, check this
    Indices.Presentation = Indices.Graphics;
    return Indices;
}

std::vector<const char*> FDevice::GetRequiredDeviceExtensions()
{
    std::vector<const char*> DeviceExtensions;
    DeviceExtensions.reserve(16);

    DeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    DeviceExtensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
    DeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
    return DeviceExtensions;
}
