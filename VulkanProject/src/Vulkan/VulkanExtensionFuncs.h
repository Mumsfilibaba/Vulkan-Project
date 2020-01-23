#pragma once

#include <vulkan/vulkan.h>

struct VkExt
{
    static PFN_vkSetDebugUtilsObjectNameEXT    vkSetDebugUtilsObjectNameEXT;
    static PFN_vkCreateDebugUtilsMessengerEXT  vkCreateDebugUtilsMessengerEXT;
    static PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
};