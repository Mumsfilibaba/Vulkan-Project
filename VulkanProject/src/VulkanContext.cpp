#include "VulkanContext.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <set>
#include <assert.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) 
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		std::cout << "Validation layer: " << pCallbackData->pMessage << std::endl;
	}
	return VK_FALSE;
}

VulkanContext* VulkanContext::Create(const DeviceParams& params)
{
	VulkanContext* pContext = new VulkanContext();
	return pContext->Init(params) ? pContext : nullptr;
}

void VulkanContext::Destroy()
{
	delete this;
}

VulkanContext::VulkanContext()
    : m_Instance(VK_NULL_HANDLE),
	m_DebugMessenger(VK_NULL_HANDLE),
	m_PhysicalDevice(VK_NULL_HANDLE),
	m_Device(VK_NULL_HANDLE),
	m_GraphicsQueue(VK_NULL_HANDLE),
	m_ComputeQueue(VK_NULL_HANDLE),
	m_TransferQueue(VK_NULL_HANDLE),
	m_Surface(VK_NULL_HANDLE),
	m_SwapChain(VK_NULL_HANDLE),
	m_Format(),
	m_Extent(),
	m_PresentMode(),
	m_EnabledDeviceFeatures(),
	m_DeviceProperties(),
	m_DeviceFeatures(),
	m_DeviceMemoryProperties(),
	m_AvailableInstanceExtension(),
	m_AvailableDeviceExtensions(),
	m_QueueFamilyIndices(),
	m_bValidationEnabled(false),
	m_bRayTracingEnabled(false)
{
}

VulkanContext::~VulkanContext()
{
	if (m_SwapChain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
		m_SwapChain = VK_NULL_HANDLE;
	}

	if (m_Device)
	{
		vkDestroyDevice(m_Device, nullptr);
		m_Device = VK_NULL_HANDLE;
	}

	if (m_Surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		m_Surface = VK_NULL_HANDLE;
	}

	if (m_bValidationEnabled)
		DestroyDebugMessenger();
	
	if (m_Instance)
	{
		vkDestroyInstance(m_Instance, nullptr);
		m_Instance = VK_NULL_HANDLE;
	}
}

bool VulkanContext::Init(const DeviceParams& params)
{
	m_bValidationEnabled = params.bEnableValidation;
	if (CreateInstance(params))
		std::cout << "Created Vulkan Instance" << std::endl;
	else
		return false;

	if (m_bValidationEnabled)
	{
		if (CreateDebugMessenger())
			std::cout << "Created Debug Messenger" << std::endl;
		else
			return false;
	}

	if (CreateSurface(params.pWindow))
		std::cout << "Created Surface" << std::endl;
	else
		return false;

	if (QueryPhysicalDevice())
		std::cout << "Queried physical device: " << m_DeviceProperties.deviceName << std::endl;
	else
		return false;

	if (CreateDeviceAndQueues(params))
		std::cout << "Created Vulkan Device" << std::endl;
	else
		return false;

	int32 width	 = 0;
	int32 height = 0;
	glfwGetWindowSize(params.pWindow, &width, &height);
	if (CreateSwapChain(uint32(width), uint32(height)))
		std::cout << "Created swapchain" << std::endl;
	else
		return false;

	return true;
}

bool VulkanContext::IsInstanceExtensionAvailable(const char* pExtensionName)
{
	for (VkExtensionProperties extension : m_AvailableInstanceExtension)
	{
		if (strcmp(extension.extensionName, pExtensionName) == 0)
			return true;
	}

	return false;
}

bool VulkanContext::IsDeviceExtensionAvailable(const char* pExtensionName)
{
	for (VkExtensionProperties extension : m_AvailableDeviceExtensions)
	{
		if (strcmp(extension.extensionName, pExtensionName) == 0)
			return true;
	}

	return false;
}

bool VulkanContext::CreateInstance(const DeviceParams& params)
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Project";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;


	//Enable GLFW extensions
	std::vector<const char*> instanceExtensions;

	uint32 requiredInstanceExtensionCount = 0;
	const char** ppRequiredInstanceExtension = glfwGetRequiredInstanceExtensions(&requiredInstanceExtensionCount);
	if (requiredInstanceExtensionCount > 0)
	{
		std::cout << "Required instance extensions:" << std::endl;
		for (uint32 i = 0; i < requiredInstanceExtensionCount; i++)
		{
			std::cout << "   " << ppRequiredInstanceExtension[i] << std::endl;
			instanceExtensions.push_back(ppRequiredInstanceExtension[i]);
		}
	}
	else
	{
		return false;
	}

	//Setup instance
	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &appInfo;

	if (m_bValidationEnabled)
		instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	//Validate extensions
	{
		uint32_t instanceExtensionCount;
		vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
		std::vector<VkExtensionProperties> instanceExtensionProperties(instanceExtensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, instanceExtensionProperties.data());
		m_AvailableInstanceExtension.swap(instanceExtensionProperties);

		std::cout << "Abailable instance extensions:" << std::endl;
		for (VkExtensionProperties extension : m_AvailableInstanceExtension)
			std::cout << "   " << extension.extensionName << std::endl;
	}

	std::cout << "Enabled instance extensions:" << std::endl;
	for (const char* pEnabledExtension : instanceExtensions)
	{
		std::cout << "   " << pEnabledExtension << std::endl;
		if (!IsInstanceExtensionAvailable(pEnabledExtension))
		{
			std::cout << "Extension '" << pEnabledExtension << "' not present" << std::endl;
			return false;
		}
	}

	instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
	instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

	//Setup validation layer
	VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = {};
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
			std::cout << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled";
		}
	}

	VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_Instance);
	if (result != VK_SUCCESS)
	{
		std::cout << "vkCreateInstance failed" << std::endl;
		return false;
	}

	return true;
}

bool VulkanContext::CreateDebugMessenger()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	PopulateDebugMessengerCreateInfo(createInfo);

	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) 
	{
		VkResult result = func(m_Instance, &createInfo, nullptr, &m_DebugMessenger);
		if (result != VK_SUCCESS)
		{
			std::cout << "vkCreateDebugUtilsMessengerEXT failed" << std::endl;
		}
		else
		{
			return true;
		}
	}
	
	return false;
}

void VulkanContext::DestroyDebugMessenger()
{
	PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) 
	{
		func(m_Instance, m_DebugMessenger, nullptr);
	}
}

void VulkanContext::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = VulkanDebugCallback;
	createInfo.pUserData = nullptr;
}

bool VulkanContext::QueryPhysicalDevice()
{
	//Enumerate devices
	uint32_t gpuCount = 0;
	VkResult result = vkEnumeratePhysicalDevices(m_Instance, &gpuCount, nullptr);
	std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
	result = vkEnumeratePhysicalDevices(m_Instance, &gpuCount, physicalDevices.data());
	if (result != VK_SUCCESS || gpuCount < 1) 
	{
		std::cerr << "Could not enumerate physical devices" << std::endl;
		return false;
	}

	//Start with the first one in case we do not find any suitable
	m_PhysicalDevice = physicalDevices[0];

	//GPU selection
	std::cout << "Available GPUs" << std::endl;
	for (VkPhysicalDevice physicalDevice : physicalDevices)
	{
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
		
		std::cout << "   " << physicalDeviceProperties.deviceName << std::endl;

		VkPhysicalDeviceFeatures physicalDeviceFeatures;
		vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

		//Check for adapter features
		if (!physicalDeviceFeatures.samplerAnisotropy)
		{
			std::cout << "Anisotropic filtering is not supported by adapter" << std::endl;
			continue;
		}

		//Find indices for queuefamilies
		QueueFamilyIndices indices = GetQueueFamilyIndices(physicalDevice);
		if (!indices.IsValid())
		{
			std::cout << "Failed to find a suitable queuefamilies" << std::endl;
			return false;
		}

		//Check if required extension for device is supported
		std::vector<const char*> deviceExtensions = GetRequiredDeviceExtensions();

		uint32_t deviceExtensionCount;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr);
		std::vector<VkExtensionProperties> availableDeviceExtension(deviceExtensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, availableDeviceExtension.data());
        
#ifdef DEBUG
        std::cout << "      Available extensions:" << std::endl;
        for (const auto& extension : availableDeviceExtension)
            std::cout << "         " << extension.extensionName << std::endl;
#endif
        
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
                std::cout << extensionName << " is not supported" << std::endl;
            }
		}

        if (extensionsFound)
        {
			//If we came this far we have found a suitable adapter
			m_PhysicalDevice = physicalDevice;
			break;
		}
        else
        {
            std::cout << "Some extensions were not supported on '" <<physicalDeviceProperties.deviceName << "'" << std::endl;
        }
	}

	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_DeviceProperties);
	vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_DeviceFeatures);
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_DeviceMemoryProperties);

	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
	
	if (!(queueFamilyCount > 0))
		return false;

	m_QueueFamilyProperties.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, m_QueueFamilyProperties.data());
	return true;
}

//Helper function
static uint32 GetQueueFamilyIndex(VkQueueFlagBits queueFlags, const std::vector<VkQueueFamilyProperties>& queueFamilies)
{
	if (queueFlags & VK_QUEUE_COMPUTE_BIT)
	{
		for (uint32 i = 0; i < uint32(queueFamilies.size()); i++)
		{
			if ((queueFamilies[i].queueFlags & queueFlags) && ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
				return i;
		}
	}

	if (queueFlags & VK_QUEUE_TRANSFER_BIT)
	{
		for (uint32 i = 0; i < uint32(queueFamilies.size()); i++)
		{
			if ((queueFamilies[i].queueFlags & queueFlags) && ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
				return i;
		}
	}

	for (uint32 i = 0; i < uint32(queueFamilies.size()); i++)
	{
		if (queueFamilies[i].queueFlags & queueFlags)
			return i;
	}

	return UINT32_MAX;
}

QueueFamilyIndices VulkanContext::GetQueueFamilyIndices(VkPhysicalDevice physicalDevice)
{
	uint32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	QueueFamilyIndices indices = {};
	indices.Compute		= GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT, queueFamilies);
	indices.Transfer	= GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT, queueFamilies);
	indices.Graphics	= GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT, queueFamilies);

	//Find a presentation queue
	for (uint32 i = 0; i < queueFamilyCount; i++)
	{
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, m_Surface, &presentSupport);
		if (presentSupport)
		{
			indices.Presentation = i;
			break;
		}
	}

	return indices;
}

std::vector<const char*> VulkanContext::GetRequiredDeviceExtensions()
{
	std::vector<const char*> deviceExtensions;
	deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	return deviceExtensions;
}

bool VulkanContext::CreateDeviceAndQueues(const DeviceParams& params)
{
	m_QueueFamilyIndices = GetQueueFamilyIndices(m_PhysicalDevice);

	std::cout << "Using following queueFamilyIndices: ";
	std::cout << "Graphics = " << m_QueueFamilyIndices.Graphics;
	std::cout << ", Presentation=" << m_QueueFamilyIndices.Presentation;
	std::cout << ", Compute=" << m_QueueFamilyIndices.Compute;
	std::cout << ", Transfer=" << m_QueueFamilyIndices.Transfer << std::endl;

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	const float defaultQueuePriority = 0.0f;

	std::set<uint32> uniqueQueueFamilies =
	{
		m_QueueFamilyIndices.Graphics,
		m_QueueFamilyIndices.Compute,
		m_QueueFamilyIndices.Presentation,
		m_QueueFamilyIndices.Transfer
	};

	for (int32 queueFamiliy : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueInfo = {};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.pNext = nullptr;
		queueInfo.flags = 0;
		queueInfo.pQueuePriorities = &defaultQueuePriority;
		queueInfo.queueFamilyIndex = queueFamiliy;
		queueInfo.queueCount = 1;

		queueCreateInfos.push_back(queueInfo);
	}

	//Get device extensions
	{
		uint32_t deviceExtensionCount;
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &deviceExtensionCount, nullptr);
		std::vector<VkExtensionProperties> availableDeviceExtension(deviceExtensionCount);
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &deviceExtensionCount, availableDeviceExtension.data());
		m_AvailableDeviceExtensions.swap(availableDeviceExtension);

		std::cout << "Abailable device extensions:" << std::endl;
		for (VkExtensionProperties extension : m_AvailableDeviceExtensions)
			std::cout << "   " << extension.extensionName << std::endl;
	}

	//Enable device extensions
	std::vector<const char*> deviceExtensions = GetRequiredDeviceExtensions();
	if (params.bEnableRayTracing)
		deviceExtensions.push_back(VK_NV_RAY_TRACING_EXTENSION_NAME);

	//TODO: Enable wanted features here

	//Create the logical device
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.pEnabledFeatures = &m_EnabledDeviceFeatures;
	
	//Verify extensions
	if (deviceExtensions.size() > 0)
	{
		std::cout << "Enabled device extensions:" << std::endl;
		for (auto it = deviceExtensions.begin(); it != deviceExtensions.end();)
		{
            if (!IsDeviceExtensionAvailable((*it)))
            {
                std::cout << "WARNING: Extension '" << (*it) << "' not present" << std::endl;
                it = deviceExtensions.erase(it);
            }
            else
            {
                std::cout << "   " << (*it) << std::endl;
                it++;
            }
		}

		deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
	}

	VkResult result = vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device);
	if (result == VK_SUCCESS)
	{
		vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.Graphics, 0, &m_GraphicsQueue);
		if (m_QueueFamilyIndices.Graphics != m_QueueFamilyIndices.Presentation)
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

bool VulkanContext::CreateSurface(GLFWwindow* pWindow)
{
	if (glfwCreateWindowSurface(m_Instance, pWindow, nullptr, &m_Surface) != VK_SUCCESS)
	{
		std::cout << "Failed to create a surface" << std::endl;
		return false;
	}

	return true;
}

bool VulkanContext::CreateSwapChain(uint32 width, uint32 height)
{
	//Get capabilities and formats that are supported
	VkSurfaceCapabilitiesKHR capabilities = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &capabilities);
	if (capabilities.currentExtent.width != UINT32_MAX && capabilities.currentExtent.height != UINT32_MAX)
	{
		m_Extent = capabilities.currentExtent;
	}
	else
	{
		VkExtent2D actualExtent = { width, height };
		actualExtent.width	= std::max(capabilities.minImageExtent.width,	std::min(capabilities.maxImageExtent.width,		actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height,	std::min(capabilities.maxImageExtent.height,	actualExtent.height));
		m_Extent = actualExtent;
	}


	std::vector<VkSurfaceFormatKHR> formats;
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, nullptr);
	if (formatCount > 0)
	{
		formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, formats.data());

		for (const auto& availableFormat : formats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				m_Format = availableFormat;
		}
	}
	else
	{
		std::cout << "No available formats for SwapChain" << std::endl;
		return false;
	}

	std::vector<VkPresentModeKHR> presentModes;
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, nullptr);
	if (presentModeCount > 0)
	{
		presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, presentModes.data());

		//Choose mailbox if available otherwise fifo
		m_PresentMode = VK_PRESENT_MODE_FIFO_KHR;
		for (const VkPresentModeKHR& availablePresentMode : presentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				m_PresentMode = availablePresentMode;
		}
	}
	else
	{
		std::cout << "No available presentModes for SwapChain" << std::endl;
		return false;
	}

	uint32_t imageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
	{
		imageCount = capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_Surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = m_Format.format;
	createInfo.imageColorSpace = m_Format.colorSpace;
	createInfo.imageExtent = m_Extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.preTransform = capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = m_PresentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_SwapChain);
	if (result != VK_SUCCESS)
	{
		std::cout << "vkCreateSwapchainKHR failed" << std::endl;
		return false;
	}

	return true;
}
