#pragma once
#include "DeviceChild.h"
#include <vulkan/vulkan.h>

class FRenderPass;
class FShaderModule;
class FPipelineLayout;

struct FGraphicsPipelineStateParams
{
    VkVertexInputAttributeDescription* pAttributeDescriptions    = nullptr;
    uint32_t                           AttributeDescriptionCount = 0;
    VkVertexInputBindingDescription*   pBindingDescriptions      = nullptr;
    uint32_t                           BindingDescriptionCount   = 0;
    VkCullModeFlagBits                 CullMode                  = VK_CULL_MODE_BACK_BIT;
    VkFrontFace                        FrontFace                 = VK_FRONT_FACE_CLOCKWISE;
    VkPrimitiveTopology                Topology                  = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPolygonMode                      PolygonMode               = VK_POLYGON_MODE_FILL;
    bool                               bBlendEnable              = false;
    bool                               bDepthEnable              = false;
    FRenderPass*                       pRenderPass               = nullptr;
    FPipelineLayout*                   pPipelineLayout           = nullptr;
    FShaderModule*                     pVertexShader             = nullptr;
    FShaderModule*                     pFragmentShader           = nullptr;
};

class FBasePipeline : public FDeviceChild
{
public:
    FBasePipeline(FDevice* pDevice);
    ~FBasePipeline();
    
    void SetDebugName(const char* DebugName);
    
    VkPipeline GetPipeline() const
    {
        return m_Pipeline;
    }
    
protected:
    VkPipeline m_Pipeline;
};

class FGraphicsPipeline : public FBasePipeline
{
public:
    static FGraphicsPipeline* Create(FDevice* pDevice, const FGraphicsPipelineStateParams& Params);
    
    FGraphicsPipeline(FDevice* pDevice);
    ~FGraphicsPipeline() = default;
};

struct FComputePipelineStateParams
{
    FShaderModule*   pShader         = nullptr;
    FPipelineLayout* pPipelineLayout = nullptr;
};

class FComputePipeline : public FBasePipeline
{
public:
    static FComputePipeline* Create(class FDevice* pDevice, const FComputePipelineStateParams& Params);
    
    FComputePipeline(FDevice* pDevice);
    ~FComputePipeline() = default;
};

struct FRayTracingPipelineStateParams
{
    FShaderModule*   pRayGenShader                = nullptr;
    FShaderModule*   pRayMissShader               = nullptr;
    FShaderModule*   pRayClosestHitShader         = nullptr;
    FPipelineLayout* pPipelineLayout              = nullptr;
    uint32_t         MaxPipelineRayRecursionDepth = 1;
};

class FRayTracingPipeline : public FBasePipeline
{
public:
    static FRayTracingPipeline* Create(class FDevice* pDevice, const FRayTracingPipelineStateParams& Params);

    FRayTracingPipeline(FDevice* pDevice);
    ~FRayTracingPipeline();

    const VkStridedDeviceAddressRegionKHR* GetRayGenSBT()      const { return &m_RayGenSBT; }
    const VkStridedDeviceAddressRegionKHR* GetRayMissSBT()     const { return &m_RayMissSBT; }
    const VkStridedDeviceAddressRegionKHR* GetRayHitSBT()      const { return &m_RayClosestHitSBT; }
    const VkStridedDeviceAddressRegionKHR* GetRayCallableSBT() const { return &m_RayCallableSBT; }

private:
    VkBuffer                        m_SBTBuffer;
    uint64_t                        m_SBTDeviceAddress;
    VkDeviceMemory                  m_SBTDeviceMemory;
    void*                           m_pShaderBindingTable;
    VkStridedDeviceAddressRegionKHR m_RayGenSBT;
    VkStridedDeviceAddressRegionKHR m_RayMissSBT;
    VkStridedDeviceAddressRegionKHR m_RayClosestHitSBT;
    VkStridedDeviceAddressRegionKHR m_RayCallableSBT;
};