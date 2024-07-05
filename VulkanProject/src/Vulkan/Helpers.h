#pragma once
#include "Core.h"
#include "Extensions.h"

inline void SetDebugName(VkDevice Device, const std::string& Name, uint64_t Handle, VkObjectType ObjectType)
{
    if (FExtensions::vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT DebugNameInfo;
        ZERO_STRUCT(&DebugNameInfo);
        
        DebugNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        DebugNameInfo.objectType   = ObjectType;
        DebugNameInfo.pObjectName  = Name.c_str();
        DebugNameInfo.objectHandle = Handle;

        VkResult Result = FExtensions::vkSetDebugUtilsObjectNameEXT(Device, &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            std::cout << "Failed to set name '" << DebugNameInfo.pObjectName << "'.Error: " << Result << std::endl;
        }
    }
}

inline uint32_t FindMemoryType(VkPhysicalDevice PhysicalDevice, uint32_t TypeFilter, VkMemoryPropertyFlags Properties)
{
    VkPhysicalDeviceMemoryProperties MemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);

    for (uint32_t i = 0; i < MemoryProperties.memoryTypeCount; i++) 
    {
        if (TypeFilter & (1 << i) && (MemoryProperties.memoryTypes[i].propertyFlags & Properties) == Properties)
        {
            return i;
        }
    }

    return UINT32_MAX;
}
