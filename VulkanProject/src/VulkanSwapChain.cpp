#include "VulkanSwapChain.h"

#include <iostream>
#include <vector>
#include <algorithm>

VulkanSwapChain::VulkanSwapChain()
	: m_Surface(VK_NULL_HANDLE),
	m_SwapChain(VK_NULL_HANDLE),
	m_Format(),
	m_Extent(),
	m_PresentMode()
{
}

bool VulkanSwapChain::Init(GLFWwindow* pWindow, VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, const QueueFamilyIndices& familyIndices)
{
	//Create surface
	if (glfwCreateWindowSurface(instance, pWindow, nullptr, &m_Surface) != VK_SUCCESS)
	{
		std::cout << "Failed to create a surface" << std::endl;
		return false;
	}

	//Check presentation support
	VkBool32 presentSupport = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, familyIndices.Graphics, m_Surface, &presentSupport);
	if (!presentSupport)
	{
		std::cout << "PhysicalDevice does not support presentation with queuefamily '" << familyIndices.Graphics << "'" << std::endl;
		return false;
	}

	//Get capabilities and formats that are supported
	VkSurfaceCapabilitiesKHR capabilities = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &capabilities);
	if (capabilities.currentExtent.width != UINT32_MAX && capabilities.currentExtent.height != UINT32_MAX)
	{
		m_Extent = capabilities.currentExtent;
	}
	else 
	{
		int width = 0;
		int height = 0;
		glfwGetWindowSize(pWindow, &width, &height);

        VkExtent2D actualExtent = { uint32(width), uint32(height) };
		actualExtent.width	= std::max(capabilities.minImageExtent.width,   std::min(capabilities.maxImageExtent.width,     actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height,  std::min(capabilities.maxImageExtent.height,    actualExtent.height));
		m_Extent = actualExtent;
	}


	std::vector<VkSurfaceFormatKHR> formats;
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, nullptr);
	if (formatCount > 0)
	{
		formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, formats.data());

		for (const auto& availableFormat : formats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
			{
				m_Format = availableFormat;
			}
		}
	}
	else
	{
		std::cout << "No available formats for SwapChain" << std::endl;
		return false;
	}

	std::vector<VkPresentModeKHR> presentModes;
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &presentModeCount, nullptr);
	if (presentModeCount > 0) 
	{
		presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &presentModeCount, presentModes.data());

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

	VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_SwapChain);
	return result == VK_SUCCESS;
}

void VulkanSwapChain::Destroy(VkInstance instance, VkDevice device)
{
	if (m_SwapChain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(device, m_SwapChain, nullptr);
		m_SwapChain = VK_NULL_HANDLE;
	}

	if (m_Surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(instance, m_Surface, nullptr);
		m_Surface = VK_NULL_HANDLE;
	}
}

void VulkanSwapChain::Present()
{
}

