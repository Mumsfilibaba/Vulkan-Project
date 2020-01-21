#include "VulkanContext.h"
#include "VulkanSwapChain.h"

#include <iostream>
#include <vector>
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

VulkanContext::VulkanContext()
    : m_Instance(VK_NULL_HANDLE),
	m_DebugMessenger(VK_NULL_HANDLE),
	m_PhysicalDevice(VK_NULL_HANDLE),
	m_Device(VK_NULL_HANDLE),
	m_GraphicsQueue(VK_NULL_HANDLE),
	m_ComputeQueue(VK_NULL_HANDLE),
	m_TransferQueue(VK_NULL_HANDLE),
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
	if (m_Device)
	{
		vkDestroyDevice(m_Device, nullptr);
		m_Device = VK_NULL_HANDLE;
	}

	if (m_bValidationEnabled)
		DestroyDebugMessenger();
	
	if (m_Instance)
	{
		vkDestroyInstance(m_Instance, nullptr);
		m_Instance = VK_NULL_HANDLE;
	}
}

VulkanContext* VulkanContext::Create(const ContextParams& params)
{
	VulkanContext* pContext = new VulkanContext();
	return pContext->Init(params) ? pContext : nullptr;
}

VulkanSwapChain* VulkanContext::CreateSwapChain(GLFWwindow* pWindow)
{
	VulkanSwapChain* pSwapChain = new VulkanSwapChain();
	return pSwapChain->Init(pWindow, m_Instance, m_PhysicalDevice, m_Device, m_QueueFamilyIndices) ? pSwapChain : nullptr;
}

void VulkanContext::DestroySwapChain(VulkanSwapChain** ppSwapChain)
{
	(*ppSwapChain)->Destroy(m_Instance, m_Device);
	delete (*ppSwapChain);
	(*ppSwapChain) = nullptr;
}

bool VulkanContext::Init(const ContextParams& params)
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

	if (QueryPhysicalDevice())
		std::cout << "Queried physical device: " << m_DeviceProperties.deviceName << std::endl;
	else
		return false;

	if (CreateDeviceAndQueues(params))
		std::cout << "Created Vulkan Device" << std::endl;
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

void VulkanContext::Release()
{
	delete this;
}

bool VulkanContext::CreateInstance(const ContextParams& params)
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Project";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	std::vector<const char*> instanceExtensions;

	//Enable GLFW extensions
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
	return result == VK_SUCCESS;
}

bool VulkanContext::CreateDebugMessenger()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	PopulateDebugMessengerCreateInfo(createInfo);

	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) 
	{
		VkResult result = func(m_Instance, &createInfo, nullptr, &m_DebugMessenger);
		return result == VK_SUCCESS;
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
	//Physical device
	uint32_t gpuCount = 0;

	VkResult result = vkEnumeratePhysicalDevices(m_Instance, &gpuCount, nullptr);
	if (result != VK_SUCCESS)
		return false;

	assert(gpuCount > 0);

	//Enumerate devices
	std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
	result = vkEnumeratePhysicalDevices(m_Instance, &gpuCount, physicalDevices.data());
	if (result != VK_SUCCESS) 
	{
		std::cerr << "Could not enumerate physical devices" << std::endl;
		return false;
	}
	else
	{
		std::cout << "Available GPUs" << std::endl;

		VkPhysicalDeviceProperties deviceProperties = {};
		for (VkPhysicalDevice physicalDevice : physicalDevices)
		{
			vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
			std::cout << "   " << deviceProperties.deviceName << std::endl;
		}
	}

	//GPU selection and get properties of this device
	m_PhysicalDevice = physicalDevices[0];
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

uint32 VulkanContext::GetQueueFamilyIndex(VkQueueFlagBits queueFlags)
{
	if (queueFlags & VK_QUEUE_COMPUTE_BIT)
	{
		for (uint32 i = 0; i < uint32(m_QueueFamilyProperties.size()); i++)
		{
			if ((m_QueueFamilyProperties[i].queueFlags & queueFlags) && ((m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
				return i;
		}
	}

	if (queueFlags & VK_QUEUE_TRANSFER_BIT)
	{
		for (uint32 i = 0; i < uint32(m_QueueFamilyProperties.size()); i++)
		{
			if ((m_QueueFamilyProperties[i].queueFlags & queueFlags) && ((m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
				return i;
		}
	}

	for (uint32 i = 0; i < uint32(m_QueueFamilyProperties.size()); i++)
	{
		if (m_QueueFamilyProperties[i].queueFlags & queueFlags)
			return i;
	}

	return VK_NULL_HANDLE;
}

bool VulkanContext::CreateDeviceAndQueues(const ContextParams& params)
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	const float defaultQueuePriority = 0.0f;

	//Graphics queue
	m_QueueFamilyIndices.Graphics = GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
	{
		VkDeviceQueueCreateInfo queueInfo = {};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = m_QueueFamilyIndices.Graphics;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &defaultQueuePriority;
		queueCreateInfos.push_back(queueInfo);
	}

	//Compute queue
	m_QueueFamilyIndices.Compute = GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
	if (m_QueueFamilyIndices.Compute != m_QueueFamilyIndices.Graphics)
	{
		VkDeviceQueueCreateInfo queueInfo = {};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = m_QueueFamilyIndices.Compute;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &defaultQueuePriority;
		queueCreateInfos.push_back(queueInfo);
	}

	//Transfer queue
	m_QueueFamilyIndices.Transfer = GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
	if ((m_QueueFamilyIndices.Transfer != m_QueueFamilyIndices.Graphics) && (m_QueueFamilyIndices.Transfer != m_QueueFamilyIndices.Compute))
	{
		VkDeviceQueueCreateInfo queueInfo = {};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = m_QueueFamilyIndices.Transfer;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &defaultQueuePriority;
		queueCreateInfos.push_back(queueInfo);
	}

	//Get device extensions
	{
		uint32_t deviceExtensionCount;
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &deviceExtensionCount, nullptr);
		std::vector<VkExtensionProperties> deviceExtensionProperties(deviceExtensionCount);
		vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &deviceExtensionCount, deviceExtensionProperties.data());
		m_AvailableDeviceExtensions.swap(deviceExtensionProperties);

		std::cout << "Abailable device extensions:" << std::endl;
		for (VkExtensionProperties extension : m_AvailableDeviceExtensions)
			std::cout << "   " << extension.extensionName << std::endl;
	}

	//Enable device extensions
	std::vector<const char*> deviceExtensions;
	deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
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
		for (auto it = deviceExtensions.begin(); it != deviceExtensions.end(); it++)
		{
			std::cout << "   " << (*it) << std::endl;
			if (!IsDeviceExtensionAvailable((*it)))
			{
				std::cout << "WARNING: Extension '" << (*it) << "' not present" << std::endl;
				it = deviceExtensions.erase(it);
			}
		}

		deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
	}

	VkResult result = vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device);
	if (result == VK_SUCCESS)
	{
		vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.Graphics, 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.Transfer, 0, &m_TransferQueue);
		vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.Compute, 0, &m_ComputeQueue);

		return true;
	}
	else
	{
		return false;
	}
}
