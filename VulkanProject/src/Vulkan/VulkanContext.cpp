#include "VulkanContext.h"
#include "VulkanHelper.h"
#include "VulkanBuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanShaderModule.h"
#include "VulkanPipelineState.h"
#include "VulkanCommandBuffer.h"
#include "VulkanExtensionFuncs.h"
#include "VulkanDeviceAllocator.h"
#include "VulkanDescriptorPool.h"

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
	m_SwapChainFormat(),
	m_Extent(),
	m_PresentMode(),
	m_EnabledDeviceFeatures(),
	m_DeviceProperties(),
	m_DeviceFeatures(),
	m_DeviceMemoryProperties(),
	m_QueueFamilyIndices(),
    m_FrameData(),
    m_FrameCount(0),
    m_SemaphoreIndex(0),
    m_CurrentBufferIndex(0),
	m_bValidationEnabled(false),
	m_bRayTracingEnabled(false)
{
}

VulkanContext::~VulkanContext()
{
    for (FrameData& frame : m_FrameData)
    {
        if (frame.ImageSemaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(m_Device, frame.ImageSemaphore, nullptr);
            frame.ImageSemaphore = VK_NULL_HANDLE;
        }
        
        if (frame.RenderSemaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(m_Device, frame.RenderSemaphore, nullptr);
            frame.RenderSemaphore = VK_NULL_HANDLE;
        }
    }
    
    ReleaseSwapChainResources();

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
    {
        if (VkExt::vkDestroyDebugUtilsMessengerEXT)
        {
			VkExt::vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
            m_DebugMessenger = nullptr;
        }
    }
	
	if (m_Instance)
	{
		vkDestroyInstance(m_Instance, nullptr);
		m_Instance = VK_NULL_HANDLE;
	}
}

VulkanBuffer* VulkanContext::CreateBuffer(const BufferParams& params, VulkanDeviceAllocator* pAllocator)
{
	return new VulkanBuffer(m_Device, m_PhysicalDevice, params, pAllocator);
}

VulkanRenderPass* VulkanContext::CreateRenderPass(const RenderPassParams& params)
{
	return new VulkanRenderPass(m_Device, params);
}

VulkanFramebuffer* VulkanContext::CreateFrameBuffer(const FramebufferParams& params)
{
	return new VulkanFramebuffer(m_Device, params);
}

VulkanShaderModule* VulkanContext::CreateShaderModule(const ShaderModuleParams& params)
{
	return new VulkanShaderModule(m_Device, params);
}

VulkanCommandBuffer* VulkanContext::CreateCommandBuffer(const CommandBufferParams& params)
{
	uint32_t queueFamilyIndex = 0;
	if (params.QueueType == ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS)
		queueFamilyIndex = m_QueueFamilyIndices.Graphics;
	else if (params.QueueType == ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE)
		queueFamilyIndex = m_QueueFamilyIndices.Compute;
	else if (params.QueueType == ECommandQueueType::COMMAND_QUEUE_TYPE_TRANSFER)
		queueFamilyIndex = m_QueueFamilyIndices.Transfer;

	return new VulkanCommandBuffer(m_Device, queueFamilyIndex, params);
}

VulkanDeviceAllocator* VulkanContext::CreateDeviceAllocator()
{
	return new VulkanDeviceAllocator(m_Device, m_PhysicalDevice);
}

VulkanDescriptorPool* VulkanContext::CreateDescriptorPool(const DescriptorPoolParams &params)
{
	return new VulkanDescriptorPool(m_Device, params);
}

VulkanGraphicsPipelineState* VulkanContext::CreateGraphicsPipelineState(const GraphicsPipelineStateParams& params)
{
	return new VulkanGraphicsPipelineState(m_Device, params);
}

void VulkanContext::ExecuteGraphics(VulkanCommandBuffer* pCommandBuffer, VkPipelineStageFlags* pWaitStages)
{
	//std::cout << "ExecuteGraphics" << std::endl;

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;

	VkSemaphore waitSemaphores[] = { m_FrameData[m_SemaphoreIndex].ImageSemaphore };

	submitInfo.waitSemaphoreCount	= 1;
	submitInfo.pWaitSemaphores		= waitSemaphores;
	submitInfo.pWaitDstStageMask	= pWaitStages;

	//Calling execute with nullptr commandbuffer results in waiting for the current semaphore
	//This seems to be the only way of handling this
	VkFence fence = VK_NULL_HANDLE;
	if (pCommandBuffer)
	{
		fence = pCommandBuffer->GetFence();

		VkCommandBuffer commandBuffers[] = { pCommandBuffer->GetCommandBuffer() };
		submitInfo.pCommandBuffers = commandBuffers;
		submitInfo.commandBufferCount = 1;

		VkSemaphore signalSemaphores[] = { m_FrameData[m_SemaphoreIndex].RenderSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;
	}
	else
	{
		submitInfo.pCommandBuffers = nullptr;
		submitInfo.commandBufferCount = 0;

		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;
	}

	VkResult result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, fence);
	if (result != VK_SUCCESS)
	{
		std::cout << "vkQueueSubmit failed. Error: " << result << std::endl;
	}
}

void VulkanContext::ResizeBuffers(uint32_t width, uint32_t height)
{
	if (m_Extent.width == width && m_Extent.height == height)
		return;

	std::cout << "Resize" << std::endl;

	ReleaseSwapChainResources();
	CreateSwapChain(width, height);

	std::cout << "Resized Buffers: w=" << m_Extent.width << ", h=" << m_Extent.height << std::endl;
}

void VulkanContext::WaitForIdle()
{
	vkDeviceWaitIdle(m_Device);
}

void VulkanContext::Present()
{
	//std::cout << "Present" << std::endl;

	VkSemaphore waitSemaphores[] = { m_FrameData[m_SemaphoreIndex].RenderSemaphore };

	VkPresentInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	info.pNext = nullptr;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores	= waitSemaphores;
	info.swapchainCount		= 1;
	info.pSwapchains		= &m_SwapChain;
	info.pImageIndices		= &m_CurrentBufferIndex;
	info.pResults			= nullptr;

	VkResult result = vkQueuePresentKHR(m_PresentationQueue, &info);
	if (result == VK_SUCCESS)
	{
		//Aquire next image
		m_SemaphoreIndex = (m_SemaphoreIndex + 1) % m_FrameCount;
		result = AquireNextImage();
	}

	//if presentation and aquire image failed
	if (result != VK_SUCCESS)
	{
		if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapChain();
			std::cout << "Suboptimal or Out Of Date SwapChain. Result: " << result << std::endl;
		}
		else
		{
			std::cout << "Present Failed. Error: " << result << std::endl;
		}
	}
}

void VulkanContext::Destroy()
{
	delete this;
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

	if (QueryPhysicalDevice(params))
		std::cout << "Queried physical device: " << m_DeviceProperties.deviceName << std::endl;
	else
		return false;

	if (CreateDeviceAndQueues(params))
		std::cout << "Created Vulkan Device" << std::endl;
	else
		return false;

	InitFrameData(FRAME_COUNT);
	if (CreateSemaphores())
		std::cout << "Created Semphores and Fences" << std::endl;
	else
		return false;

    int32_t width  = 0;
    int32_t height = 0;
    glfwGetWindowSize(params.pWindow, &width, &height);
    
    if (CreateSwapChain(uint32_t(width), uint32_t(height)))
        std::cout << "Created swapchain" << std::endl;
    else
        return false;

	return true;
}

bool VulkanContext::CreateInstance(const DeviceParams& params)
{
	VkApplicationInfo appInfo = {};
	appInfo.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext               = nullptr;
	appInfo.pApplicationName    = "Vulkan Project";
	appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName         = "";
	appInfo.engineVersion       = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion          = VK_API_VERSION_1_1;

	//Enable GLFW extensions
	std::vector<const char*> instanceExtensions;

	uint32_t requiredInstanceExtensionCount = 0;
	const char** ppRequiredInstanceExtension = glfwGetRequiredInstanceExtensions(&requiredInstanceExtensionCount);
	if (requiredInstanceExtensionCount > 0)
	{
		std::cout << "Required instance extensions:" << std::endl;
		for (uint32_t i = 0; i < requiredInstanceExtensionCount; i++)
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
	uint32_t instanceExtensionCount;
	vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
	std::vector<VkExtensionProperties> instanceExtensionProperties(instanceExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, instanceExtensionProperties.data());

	if (params.bVerbose)
	{
		std::cout << "Abailable instance extensions:" << std::endl;
		for (VkExtensionProperties extension : instanceExtensionProperties)
			std::cout << "   " << extension.extensionName << std::endl;
	}

	std::cout << "Enabled instance extensions:" << std::endl;
	for (const char* pEnabledExtension : instanceExtensions)
	{
		std::cout << "   " << pEnabledExtension << std::endl;

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
			std::cout << "Extension '" << pEnabledExtension << "' not present" << std::endl;
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

    //Get instance functions
    VkExt::vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(m_Instance, "vkSetDebugUtilsObjectNameEXT");
    if (!VkExt::vkSetDebugUtilsObjectNameEXT)
        std::cout << "Failed to retrive 'vkSetDebugUtilsObjectNameEXT'" << std::endl;
	VkExt::vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
    if (!VkExt::vkCreateDebugUtilsMessengerEXT)
        std::cout << "Failed to retrive 'vkCreateDebugUtilsMessengerEXT'" << std::endl;
	VkExt::vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
    if (!VkExt::vkDestroyDebugUtilsMessengerEXT)
        std::cout << "Failed to retrive 'vkDestroyDebugUtilsMessengerEXT'" << std::endl;
    
	return true;
}

bool VulkanContext::CreateDebugMessenger()
{
    if (VkExt::vkCreateDebugUtilsMessengerEXT)
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
        PopulateDebugMessengerCreateInfo(createInfo);

        VkResult result = VkExt::vkCreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger);
		if (result != VK_SUCCESS)
        {
            std::cout << "vkCreateDebugUtilsMessengerEXT failed. Error: " << result << std::endl;
        }
		else
        {
            return true;
        }
	}
	
	return false;
}

bool VulkanContext::CreateSurface(GLFWwindow* pWindow)
{
	VkResult result = glfwCreateWindowSurface(m_Instance, pWindow, nullptr, &m_Surface);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to create a surface. Error: " << result << std::endl;
		return false;
	}

	return true;
}

bool VulkanContext::CreateDeviceAndQueues(const DeviceParams& params)
{
	m_QueueFamilyIndices = GetQueueFamilyIndices(m_PhysicalDevice);

	std::cout << "Using following queueFamilyIndices: ";
	std::cout << "Graphics = "		<< m_QueueFamilyIndices.Graphics;
	std::cout << ", Presentation="	<< m_QueueFamilyIndices.Presentation;
	std::cout << ", Compute="		<< m_QueueFamilyIndices.Compute;
	std::cout << ", Transfer="		<< m_QueueFamilyIndices.Transfer << std::endl;

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
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.pNext = nullptr;
		queueInfo.flags = 0;
		queueInfo.pQueuePriorities	= &defaultQueuePriority;
		queueInfo.queueFamilyIndex	= queueFamiliy;
		queueInfo.queueCount		= 1;

		queueCreateInfos.push_back(queueInfo);
	}

	//Get device extensions
	uint32_t deviceExtensionCount;
	vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &deviceExtensionCount, nullptr);
	std::vector<VkExtensionProperties> availableDeviceExtension(deviceExtensionCount);
	vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &deviceExtensionCount, availableDeviceExtension.data());

	if (params.bVerbose)
	{
		std::cout << "Available device extensions:" << std::endl;
		for (VkExtensionProperties extension : availableDeviceExtension)
			std::cout << "   " << extension.extensionName << std::endl;
	}

	//Enable device extensions
	std::vector<const char*> deviceExtensions = GetRequiredDeviceExtensions();
	if (params.bEnableRayTracing)
	{
		deviceExtensions.push_back(VK_NV_RAY_TRACING_EXTENSION_NAME);
		deviceExtensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
		m_bRayTracingEnabled = true;
	}

	//TODO: Enable wanted features here

	//Create the logical device
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount	= static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos		= queueCreateInfos.data();
	deviceCreateInfo.pEnabledFeatures		= &m_EnabledDeviceFeatures;

	//Verify extensions
	if (deviceExtensions.size() > 0)
	{
		std::cout << "Enabled device extensions:" << std::endl;
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

			//Warn that extension is not present, and do not try and activate it
			if (!extensionFound)
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

void VulkanContext::InitFrameData(uint32_t numFrames)
{
	m_FrameCount = numFrames;
	m_FrameData.resize(m_FrameCount);
}

bool VulkanContext::CreateSemaphores()
{
	//Setup semaphore structure
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = nullptr;
	semaphoreInfo.flags = 0;

	//Create semaphores
	for (uint32_t i = 0; i < m_FrameCount; i++)
	{
		VkSemaphore imageSemaphore = VK_NULL_HANDLE;
		VkSemaphore renderSemaphore = VK_NULL_HANDLE;

		if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &imageSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &renderSemaphore) != VK_SUCCESS)
		{
			std::cout << "vkCreateSemaphore failed" << std::endl;
			return false;
		}
		else
		{
			SetDebugName(m_Device, "ImageSemaphore[" + std::to_string(i) + "]", (uint64_t)imageSemaphore, VK_OBJECT_TYPE_SEMAPHORE);
			SetDebugName(m_Device, "RenderSemaphore[" + std::to_string(i) + "]", (uint64_t)renderSemaphore, VK_OBJECT_TYPE_SEMAPHORE);
		}

		m_FrameData[i].ImageSemaphore = imageSemaphore;
		m_FrameData[i].RenderSemaphore = renderSemaphore;
	}

	return true;
}

bool VulkanContext::CreateSwapChain(uint32_t width, uint32_t height)
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
		actualExtent.width	= std::max(capabilities.minImageExtent.width,  std::min(capabilities.maxImageExtent.width,  actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height,	actualExtent.height));
		
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
				m_SwapChainFormat = availableFormat;
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

	uint32_t imageCount = FRAME_COUNT;
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
	{
		imageCount = capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType			= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.pNext			= nullptr;
	createInfo.surface			= m_Surface;
	createInfo.minImageCount	= imageCount;
	createInfo.imageFormat		= m_SwapChainFormat.format;
	createInfo.imageColorSpace	= m_SwapChainFormat.colorSpace;
	createInfo.imageExtent		= m_Extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage		= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.preTransform		= capabilities.currentTransform;
	createInfo.compositeAlpha	= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode		= m_PresentMode;
	createInfo.clipped			= VK_TRUE;
	createInfo.oldSwapchain		= VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_SwapChain);
	if (result != VK_SUCCESS)
	{
		std::cout << "vkCreateSwapchainKHR failed. Error: " << result << std::endl;
		return false;
	}

	//Get the images and create imageviews
	uint32_t realImageCount = 0;
	vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &realImageCount, nullptr);
	if (realImageCount < m_FrameCount)
		std::cout << "WARNING: Less images than requested in swapchain" << std::endl;

	std::vector<VkImage> images(realImageCount);
	vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &realImageCount, images.data());

	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType		= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext		= nullptr;
	imageViewCreateInfo.viewType	= VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format		= m_SwapChainFormat.format;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel	= 0;
	imageViewCreateInfo.subresourceRange.levelCount		= 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount		= 1;

	for (uint32_t i = 0; i < m_FrameCount; i++)
	{
		VkImageView imageView = VK_NULL_HANDLE;
		imageViewCreateInfo.image = images[i];

		result = vkCreateImageView(m_Device, &imageViewCreateInfo, nullptr, &imageView);
		if (result != VK_SUCCESS)
		{
			std::cout << "vkCreateImageView failed. Error: " << result << std::endl;
		}
		else
		{
			std::cout << "Created ImageView" << std::endl;
		}

		m_FrameData[i].BackBuffer = images[i];
		m_FrameData[i].BackBufferView = imageView;
	}

	result = AquireNextImage();
	if (result != VK_SUCCESS)
	{
		std::cout << "AquireNextImage failed. Error: " << result << std::endl;
	}

	return true;
}

VkResult VulkanContext::AquireNextImage()
{
	//std::cout << "AquireNextImage" << std::endl;

	VkSemaphore signalSemaphore = m_FrameData[m_SemaphoreIndex].ImageSemaphore;
	return vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, signalSemaphore, VK_NULL_HANDLE, &m_CurrentBufferIndex);
}

void VulkanContext::RecreateSwapChain()
{
	vkDeviceWaitIdle(m_Device);

	ReleaseSwapChainResources();
	CreateSwapChain(0, 0);
}

void VulkanContext::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo.sType			= VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.flags			= 0;
	createInfo.pNext			= nullptr;
	createInfo.messageSeverity	= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType		= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback	= VulkanDebugCallback;
	createInfo.pUserData		= nullptr;
}

bool VulkanContext::QueryPhysicalDevice(const DeviceParams& params)
{
	//Enumerate devices
	uint32_t gpuCount = 0;
	VkResult result = vkEnumeratePhysicalDevices(m_Instance, &gpuCount, nullptr);

	std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
	result = vkEnumeratePhysicalDevices(m_Instance, &gpuCount, physicalDevices.data());
	if (result != VK_SUCCESS || gpuCount < 1) 
	{
		std::cerr << "vkEnumeratePhysicalDevices failed. Error: " << result << std::endl;
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
        
		if (params.bVerbose)
		{
			std::cout << "      Available extensions:" << std::endl;
			for (const auto& extension : availableDeviceExtension)
				std::cout << "         " << extension.extensionName << std::endl;
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
	return true;
}

//Helper function
static uint32_t GetQueueFamilyIndex(VkQueueFlagBits queueFlags, const std::vector<VkQueueFamilyProperties>& queueFamilies)
{
	if (queueFlags & VK_QUEUE_COMPUTE_BIT)
	{
		for (uint32_t i = 0; i < uint32_t(queueFamilies.size()); i++)
		{
			if ((queueFamilies[i].queueFlags & queueFlags) && ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
				return i;
		}
	}

	if (queueFlags & VK_QUEUE_TRANSFER_BIT)
	{
		for (uint32_t i = 0; i < uint32_t(queueFamilies.size()); i++)
		{
			if ((queueFamilies[i].queueFlags & queueFlags) && ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
				return i;
		}
	}

	for (uint32_t i = 0; i < uint32_t(queueFamilies.size()); i++)
	{
		if (queueFamilies[i].queueFlags & queueFlags)
			return i;
	}

	return UINT32_MAX;
}

QueueFamilyIndices VulkanContext::GetQueueFamilyIndices(VkPhysicalDevice physicalDevice)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	QueueFamilyIndices indices = {};
	indices.Compute		= GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT, queueFamilies);
	indices.Transfer	= GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT, queueFamilies);
	indices.Graphics	= GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT, queueFamilies);

	//Find a presentation queue
	for (uint32_t i = 0; i < queueFamilyCount; i++)
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

void VulkanContext::ReleaseSwapChainResources()
{
    //Release backbuffers
	for (FrameData& frame : m_FrameData)
	{
        frame.BackBuffer = VK_NULL_HANDLE;
		if (frame.BackBufferView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(m_Device, frame.BackBufferView, nullptr);
			frame.BackBufferView = VK_NULL_HANDLE;
		}
	}

    //Destroy swapchain
    if (m_SwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
        m_SwapChain = VK_NULL_HANDLE;
    }
}
