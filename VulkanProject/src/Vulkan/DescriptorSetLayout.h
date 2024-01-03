#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>

class VulkanContext;

struct DescriptorSetLayoutParams
{
    VkDescriptorSetLayoutBinding* pBindings = nullptr;
    uint32_t numBindings = 0;
};

class DescriptorSetLayout
{
public:
    static DescriptorSetLayout* Create(VulkanContext* pContext, const DescriptorSetLayoutParams& params);

    DescriptorSetLayout(VkDevice device);
    ~DescriptorSetLayout();

    VkDescriptorSetLayout GetDescriptorSetLayout() const
    {
        return m_DescriptorSetLayout;
    }

private:
    VkDevice              m_Device;
    VkDescriptorSetLayout m_DescriptorSetLayout;
};