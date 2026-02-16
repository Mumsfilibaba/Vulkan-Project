#pragma once
#include "DeviceChild.h"
#include "Buffer.h"
#include <array>

class CDevice;
class CTextureView;
class CCommandBuffer;
class CPipelineLayout;

class IBindlessManager : public CDeviceChild
{
public:
    static IBindlessManager* Create(CDevice* pDevice);

    static constexpr const uint32_t InvalidBindlessID = static_cast<uint32_t>(-1);

    IBindlessManager(CDevice* pDevice)
        : CDeviceChild(pDevice)
    {
    }

    virtual ~IBindlessManager() = default;

    // Returns a Bindless ID
    virtual uint32_t AddImageView(VkImageView ImageView, VkSampler Sampler) = 0;
    
    // Removes a ImageView and frees the Bindless ID
    virtual void RemoveImageView(VkImageView ImageView) = 0;

    // Updates shared bindless descriptors used by ray tracing / compute passes.
    virtual void BindAccelerationStructure(VkAccelerationStructureKHR AccelerationStructure, uint32_t Binding) = 0;
    virtual void BindStorageImage(VkImageView ImageView, uint32_t Binding) = 0;
    virtual void BindCombinedImageSampler(VkImageView ImageView, VkSampler Sampler, uint32_t Binding) = 0;
    virtual void BindStorageBuffer(VkBuffer Buffer, VkDeviceSize Range, uint32_t Binding) = 0;
    virtual void BindUniformBuffer(VkBuffer Buffer, VkDeviceSize Range, uint32_t Binding) = 0;

    // Bind the bindless descriptors to the command-buffer
    virtual void BindToCommandBuffer(CCommandBuffer* pCommandBuffer, CPipelineLayout* pPipelineLayout, VkPipelineBindPoint BindPoint) = 0;

    virtual VkDescriptorSetLayout GetDescriptorSetLayout() const = 0;
    virtual bool UsesDescriptorBuffer() const = 0;
};

class CDescriptorSetBindlessManager final : public IBindlessManager
{
public:
    static CDescriptorSetBindlessManager* Create(CDevice* pDevice, uint32_t MaxTextureBinding);

    CDescriptorSetBindlessManager(CDevice* pDevice);
    ~CDescriptorSetBindlessManager();

    virtual uint32_t AddImageView(VkImageView ImageView, VkSampler Sampler) override;
    virtual void RemoveImageView(VkImageView ImageView) override;
    virtual void BindAccelerationStructure(VkAccelerationStructureKHR AccelerationStructure, uint32_t Binding) override;
    virtual void BindStorageImage(VkImageView ImageView, uint32_t Binding) override;
    virtual void BindCombinedImageSampler(VkImageView ImageView, VkSampler Sampler, uint32_t Binding) override;
    virtual void BindStorageBuffer(VkBuffer Buffer, VkDeviceSize Range, uint32_t Binding) override;
    virtual void BindUniformBuffer(VkBuffer Buffer, VkDeviceSize Range, uint32_t Binding) override;

    virtual void BindToCommandBuffer(CCommandBuffer* pCommandBuffer, CPipelineLayout* pPipelineLayout, VkPipelineBindPoint BindPoint) override;
    
    virtual VkDescriptorSetLayout GetDescriptorSetLayout() const override
    {
        return m_DescriptorSetLayout;
    }

    virtual bool UsesDescriptorBuffer() const override
    {
        return false;
    }

private:
    using BindlessMap = std::unordered_map<VkImageView, uint32_t>;

    VkDescriptorSet       m_DescriptorSet;
    VkDescriptorPool      m_DescriptorPool;
    VkDescriptorSetLayout m_DescriptorSetLayout;
    uint32_t              m_NextTextureBinding;
    uint32_t              m_MaxTextureBinding;
    std::vector<uint32_t> m_TextureFreeList;
    BindlessMap           m_TextureBindings;
};

class CDescriptorBufferBindlessManager final : public IBindlessManager
{
public:
    static CDescriptorBufferBindlessManager* Create(CDevice* pDevice, uint32_t MaxTextureBinding);

    CDescriptorBufferBindlessManager(CDevice* pDevice);
    ~CDescriptorBufferBindlessManager();

    virtual uint32_t AddImageView(VkImageView ImageView, VkSampler Sampler) override;
    virtual void RemoveImageView(VkImageView ImageView) override;
    virtual void BindAccelerationStructure(VkAccelerationStructureKHR AccelerationStructure, uint32_t Binding) override;
    virtual void BindStorageImage(VkImageView ImageView, uint32_t Binding) override;
    virtual void BindCombinedImageSampler(VkImageView ImageView, VkSampler Sampler, uint32_t Binding) override;
    virtual void BindStorageBuffer(VkBuffer Buffer, VkDeviceSize Range, uint32_t Binding) override;
    virtual void BindUniformBuffer(VkBuffer Buffer, VkDeviceSize Range, uint32_t Binding) override;
    
    virtual void BindToCommandBuffer(CCommandBuffer* pCommandBuffer, CPipelineLayout* pPipelineLayout, VkPipelineBindPoint BindPoint) override;
    
    virtual VkDescriptorSetLayout GetDescriptorSetLayout() const override
    {
        return m_DescriptorSetLayout;
    }

    virtual bool UsesDescriptorBuffer() const override
    {
        return true;
    }

private:
    using BindlessMap = std::unordered_map<VkImageView, uint32_t>;
    static constexpr uint32_t MaxTrackedBindings = 18;

    VkDescriptorSetLayout m_DescriptorSetLayout;
    CBuffer*              m_pDescriptorBuffer;
    uint8_t*              m_pMappedDescriptorBuffer;
    VkBufferUsageFlags    m_DescriptorBufferUsageFlags;
    VkDeviceSize          m_DescriptorBufferSize;
    VkDeviceSize          m_DescriptorBufferSetOffset;
    std::array<VkDeviceSize, MaxTrackedBindings> m_DescriptorBufferBindingOffsets;
    uint32_t              m_DescriptorBufferCombinedImageSamplerSize;
    uint32_t              m_DescriptorBufferStorageImageSize;
    uint32_t              m_DescriptorBufferStorageBufferSize;
    uint32_t              m_DescriptorBufferUniformBufferSize;
    uint32_t              m_DescriptorBufferAccelerationStructureSize;
    uint32_t              m_NextTextureBinding;
    uint32_t              m_MaxTextureBinding;
    std::vector<uint32_t> m_TextureFreeList;
    BindlessMap           m_TextureBindings;
};
