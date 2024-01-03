#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>

class VulkanContext;
class DescriptorSetLayout;

struct PipelineLayoutParams
{
    DescriptorSetLayout** ppLayouts = nullptr;
    uint32_t numLayouts       = 0;
    uint32_t numPushConstants = 0;
};

class PipelineLayout
{
public:
    static PipelineLayout* Create(VulkanContext* pContext, const PipelineLayoutParams& params);

    PipelineLayout(VkDevice device);
    ~PipelineLayout();

    VkPipelineLayout GetPipelineLayout() const
    {
        return m_PipelineLayout;
    }

private:
    VkDevice         m_Device;
    VkPipelineLayout m_PipelineLayout;
};