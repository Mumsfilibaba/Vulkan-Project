#pragma once
#include "Core.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

class VulkanContext
{
public:
    VulkanContext();
    ~VulkanContext() = default;
    DECL_NO_COPY(VulkanContext);

    bool Init(GLFWwindow* pWindow, bool enableValidation);
private:
    void CreateInstance();
    void CreateDevice();
private:
    VkInstance m_Instance;
};