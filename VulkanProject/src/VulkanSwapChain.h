#pragma once
#include "Core.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

class VulkanSwapChain
{
	friend class VulkanContext;
public:
	void Present();
private:
	DECL_NO_COPY(VulkanSwapChain);

	VulkanSwapChain();
	~VulkanSwapChain() = default;

	bool Init(GLFWwindow* pWindow, VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, const QueueFamilyIndices& indices);
	void Destroy(VkInstance instance, VkDevice device);
private:
	VkSurfaceKHR m_Surface;
	VkSwapchainKHR m_SwapChain;
	VkSurfaceFormatKHR m_Format;
	VkExtent2D m_Extent;
	VkPresentModeKHR m_PresentMode;
};