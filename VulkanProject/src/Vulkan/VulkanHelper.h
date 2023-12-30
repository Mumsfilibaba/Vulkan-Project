#pragma once
#include "Core.h"
#include "VulkanExtensionFuncs.h"

inline void SetDebugName(VkDevice device, const std::string& name, uint64_t vulkanHandle, VkObjectType type)
{
    if (VkExt::vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT info = {};
        info.sType          = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        info.pNext          = nullptr;
        info.objectType     = type;
        info.pObjectName    = name.c_str();
        info.objectHandle   = vulkanHandle;

        VkResult result = VkExt::vkSetDebugUtilsObjectNameEXT(device, &info);
        if (result != VK_SUCCESS)
        {
            std::cout << "Failed to set name '" << info.pObjectName << "'.Error: " << result << std::endl;
        }
    }
}

inline uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
    {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    return UINT32_MAX;
}
