#include "VulkanContext.h"

#include <iostream>
#include <vector>

VulkanContext::VulkanContext()
    : m_Instance(VK_NULL_HANDLE)
{
}

VulkanContext::~VulkanContext()
{
}

bool VulkanContext::Init(GLFWwindow* pWindow, bool enableValidation)
{
    VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Project";
	appInfo.pEngineName = "";
	appInfo.apiVersion = VK_API_VERSION_1_1;

    std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };
	// Enable surface extensions depending on os
#if defined(_WIN32)
	instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
	instanceExtensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#endif

	if (enabledInstanceExtensions.size() > 0) 
    {
		for (auto enabledExtension : enabledInstanceExtensions) 
        {
			instanceExtensions.push_back(enabledExtension);
		}
	}

	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = NULL;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	if (instanceExtensions.size() > 0)
	{
		if (enableValidation)
		{
			instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
	}

	if (enableValidation)
	{
		// The VK_LAYER_KHRONOS_validation contains all current validation functionality.
		// Note that on Android this layer requires at least NDK r20 
		const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
		// Check if this layer is available at instance level
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
			instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
			instanceCreateInfo.enabledLayerCount = 1;
		} else 
        {
			std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled";
		}
	}

    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_Instance);
	return result == VK_SUCCESS;
}