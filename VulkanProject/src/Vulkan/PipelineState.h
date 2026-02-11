#pragma once
#include "DeviceChild.h"
#include <vulkan/vulkan.h>

class CRenderPass;
class CShaderModule;
class CPipelineLayout;

struct SGraphicsPipelineStateParams
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
    CRenderPass*                       pRenderPass               = nullptr;
    VkFormat*                          pColorAttachmentFormats   = nullptr;
    uint32_t                           ColorAttachmentFormatCount = 0;
    VkFormat                           DepthAttachmentFormat     = VK_FORMAT_UNDEFINED;
    VkFormat                           StencilAttachmentFormat   = VK_FORMAT_UNDEFINED;
    CPipelineLayout*                   pPipelineLayout           = nullptr;
    CShaderModule*                     pVertexShader             = nullptr;
    CShaderModule*                     pFragmentShader           = nullptr;
};

class CBasePipeline : public CDeviceChild
{
public:
    CBasePipeline(CDevice* pDevice);
    ~CBasePipeline();
    
    void SetDebugName(const char* DebugName);
    
    VkPipeline GetPipeline() const
    {
        return m_Pipeline;
    }
    
protected:
    VkPipeline m_Pipeline;
};

class CGraphicsPipeline : public CBasePipeline
{
public:
    static CGraphicsPipeline* Create(CDevice* pDevice, const SGraphicsPipelineStateParams& Params);
    
    CGraphicsPipeline(CDevice* pDevice);
    ~CGraphicsPipeline() = default;
};

struct SComputePipelineStateParams
{
    CShaderModule*   pShader         = nullptr;
    CPipelineLayout* pPipelineLayout = nullptr;
};

class CComputePipeline : public CBasePipeline
{
public:
    static CComputePipeline* Create(class CDevice* pDevice, const SComputePipelineStateParams& Params);
    
    CComputePipeline(CDevice* pDevice);
    ~CComputePipeline() = default;
};

struct SRayTracingPipelineStateParams
{
    CShaderModule*   pRayGenShader                = nullptr;
    CShaderModule*   pRayMissShader               = nullptr;
    CShaderModule*   pRayClosestHitShader         = nullptr;
    CShaderModule*   pRayAnyHitShader             = nullptr;
    CPipelineLayout* pPipelineLayout              = nullptr;
    uint32_t         MaxPipelineRayRecursionDepth = 1;
};

class CRayTracingPipeline : public CBasePipeline
{
public:
    static CRayTracingPipeline* Create(class CDevice* pDevice, const SRayTracingPipelineStateParams& Params);

    CRayTracingPipeline(CDevice* pDevice);
    ~CRayTracingPipeline();

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