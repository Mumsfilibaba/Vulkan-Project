#include "BindlessManager.h"
#include "CommandBuffer.h"
#include "Device.h"
#include "Extensions.h"
#include <cstring>

static constexpr uint32_t BindlessSampledTextureBinding = 0;
static constexpr uint32_t BindlessAccelerationStructureBinding = 1;
static constexpr uint32_t BindlessStorageOutputImageBinding = 2;
static constexpr uint32_t BindlessStoragePreviousImageBinding = 3;
static constexpr uint32_t BindlessSkyboxBinding = 4;
static constexpr uint32_t BindlessStorageBufferBeginBinding = 5;
static constexpr uint32_t BindlessStorageBufferCount = 9;
static constexpr uint32_t BindlessStorageBufferEndBinding = BindlessStorageBufferBeginBinding + BindlessStorageBufferCount - 1;
static constexpr uint32_t BindlessUniformCameraBinding = 14;
static constexpr uint32_t BindlessUniformRandomBinding = 15;
static constexpr uint32_t BindlessUniformSceneBinding = 16;
static constexpr uint32_t BindlessStorageTLASBinding = 17;
static constexpr uint32_t BindlessMaxBinding = BindlessStorageTLASBinding;

static bool CreateBindlessDescriptorSetLayout(CDevice* pDevice, uint32_t MaxTextureBinding, bool bDescriptorBufferLayout, VkDescriptorSetLayout& OutLayout)
{
    const bool bRayTracingSupported = pDevice->IsRayTracingSupported();

    VkDescriptorSetLayoutBinding Bindings[9 + BindlessStorageBufferCount] = {};
    uint32_t BindingCount = 0;

    Bindings[BindingCount].binding         = BindlessSampledTextureBinding;
    Bindings[BindingCount].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    Bindings[BindingCount].descriptorCount = MaxTextureBinding;
    Bindings[BindingCount].stageFlags      = VK_SHADER_STAGE_ALL;
    BindingCount++;

    if (bRayTracingSupported)
    {
        Bindings[BindingCount].binding         = BindlessAccelerationStructureBinding;
        Bindings[BindingCount].descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        Bindings[BindingCount].descriptorCount = 1;
        Bindings[BindingCount].stageFlags      = VK_SHADER_STAGE_ALL;
        BindingCount++;
    }

    Bindings[BindingCount].binding         = BindlessStorageOutputImageBinding;
    Bindings[BindingCount].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    Bindings[BindingCount].descriptorCount = 1;
    Bindings[BindingCount].stageFlags      = VK_SHADER_STAGE_ALL;
    BindingCount++;

    Bindings[BindingCount].binding         = BindlessStoragePreviousImageBinding;
    Bindings[BindingCount].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    Bindings[BindingCount].descriptorCount = 1;
    Bindings[BindingCount].stageFlags      = VK_SHADER_STAGE_ALL;
    BindingCount++;

    Bindings[BindingCount].binding         = BindlessSkyboxBinding;
    Bindings[BindingCount].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    Bindings[BindingCount].descriptorCount = 1;
    Bindings[BindingCount].stageFlags      = VK_SHADER_STAGE_ALL;
    BindingCount++;

    for (uint32_t Index = 0; Index < BindlessStorageBufferCount; Index++)
    {
        Bindings[BindingCount].binding         = BindlessStorageBufferBeginBinding + Index;
        Bindings[BindingCount].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        Bindings[BindingCount].descriptorCount = 1;
        Bindings[BindingCount].stageFlags      = VK_SHADER_STAGE_ALL;
        BindingCount++;
    }

    Bindings[BindingCount].binding         = BindlessUniformCameraBinding;
    Bindings[BindingCount].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    Bindings[BindingCount].descriptorCount = 1;
    Bindings[BindingCount].stageFlags      = VK_SHADER_STAGE_ALL;
    BindingCount++;

    Bindings[BindingCount].binding         = BindlessUniformRandomBinding;
    Bindings[BindingCount].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    Bindings[BindingCount].descriptorCount = 1;
    Bindings[BindingCount].stageFlags      = VK_SHADER_STAGE_ALL;
    BindingCount++;

    Bindings[BindingCount].binding         = BindlessUniformSceneBinding;
    Bindings[BindingCount].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    Bindings[BindingCount].descriptorCount = 1;
    Bindings[BindingCount].stageFlags      = VK_SHADER_STAGE_ALL;
    BindingCount++;

    Bindings[BindingCount].binding         = BindlessStorageTLASBinding;
    Bindings[BindingCount].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    Bindings[BindingCount].descriptorCount = 1;
    Bindings[BindingCount].stageFlags      = VK_SHADER_STAGE_ALL;
    BindingCount++;

    VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = {};
    LayoutCreateInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    LayoutCreateInfo.flags        = 0;
    LayoutCreateInfo.bindingCount = BindingCount;
    LayoutCreateInfo.pBindings    = Bindings;

    if (bDescriptorBufferLayout)
    {
        LayoutCreateInfo.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
    }
    else
    {
        LayoutCreateInfo.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
    }

    VkDescriptorBindingFlags BindlessFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT;

    if (!bDescriptorBufferLayout)
    {
        BindlessFlags |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
    }

    VkDescriptorBindingFlags BindingFlags[9 + BindlessStorageBufferCount] = {};
    BindingFlags[0] = BindlessFlags;

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT BindingFlagsCreateInfo = {};
    BindingFlagsCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    BindingFlagsCreateInfo.bindingCount  = BindingCount;
    BindingFlagsCreateInfo.pBindingFlags = BindingFlags;
    LayoutCreateInfo.pNext               = &BindingFlagsCreateInfo;

    return vkCreateDescriptorSetLayout(pDevice->GetDevice(), &LayoutCreateInfo, nullptr, &OutLayout) == VK_SUCCESS;
}

IBindlessManager* IBindlessManager::Create(CDevice* pDevice)
{
    const VkPhysicalDeviceLimits& DeviceLimits = pDevice->GetDeviceLimits();
    if (DeviceLimits.maxPerStageDescriptorSampledImages < 16)
    {
        LOG("Not enough samplers/images supported for bindless\n");
        return nullptr;
    }

    const uint32_t MaxTextureBinding = static_cast<uint32_t>(DeviceLimits.maxPerStageDescriptorSampledImages - 16);
    if (pDevice->IsDescriptorBufferSupported())
    {
        IBindlessManager* pManager = CDescriptorBufferBindlessManager::Create(pDevice, MaxTextureBinding);
        if (pManager)
        {
            LOG("Using DescriptorBuffer bindless backend\n");
            return pManager;
        }

        LOG("DescriptorBuffer bindless backend creation failed. Falling back to descriptor-set bindless.\n");
    }

    IBindlessManager* pManager = CDescriptorSetBindlessManager::Create(pDevice, MaxTextureBinding);
    if (!pManager)
    {
        return nullptr;
    }

    LOG("Using DescriptorSet bindless backend\n");
    return pManager;
}

CDescriptorSetBindlessManager* CDescriptorSetBindlessManager::Create(CDevice* pDevice, uint32_t MaxTextureBinding)
{
    CDescriptorSetBindlessManager* pBindlessManager = new CDescriptorSetBindlessManager(pDevice);
    pBindlessManager->m_MaxTextureBinding = MaxTextureBinding;

    if (!CreateBindlessDescriptorSetLayout(pDevice, MaxTextureBinding, false, pBindlessManager->m_DescriptorSetLayout))
    {
        LOG("Bindless vkCreateDescriptorSetLayout failed\n");
        SAFE_DELETE(pBindlessManager);
        return nullptr;
    }

    VkDescriptorPoolSize PoolSizes[5] = {};
    uint32_t PoolSizeCount = 0;

    PoolSizes[0].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    PoolSizes[0].descriptorCount = MaxTextureBinding + 1;
    PoolSizeCount++;

    if (pDevice->IsRayTracingSupported())
    {
        PoolSizes[PoolSizeCount].type            = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        PoolSizes[PoolSizeCount].descriptorCount = 1;
        PoolSizeCount++;
    }

    PoolSizes[PoolSizeCount].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    PoolSizes[PoolSizeCount].descriptorCount = 2;
    PoolSizeCount++;

    PoolSizes[PoolSizeCount].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    PoolSizes[PoolSizeCount].descriptorCount = BindlessStorageBufferCount + 1;
    PoolSizeCount++;

    PoolSizes[PoolSizeCount].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    PoolSizes[PoolSizeCount].descriptorCount = 3;
    PoolSizeCount++;

    VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo = {};
    DescriptorPoolCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    DescriptorPoolCreateInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
    DescriptorPoolCreateInfo.poolSizeCount = PoolSizeCount;
    DescriptorPoolCreateInfo.pPoolSizes    = PoolSizes;
    DescriptorPoolCreateInfo.maxSets       = 1;

    VkResult Result = vkCreateDescriptorPool(pDevice->GetDevice(), &DescriptorPoolCreateInfo, nullptr, &pBindlessManager->m_DescriptorPool);
    if (Result != VK_SUCCESS)
    {
        LOG("Bindless vkCreateDescriptorPool failed\n");
        SAFE_DELETE(pBindlessManager);
        return nullptr;
    }

    VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo = {};
    DescriptorSetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DescriptorSetAllocateInfo.descriptorSetCount = 1;
    DescriptorSetAllocateInfo.pSetLayouts        = &pBindlessManager->m_DescriptorSetLayout;
    DescriptorSetAllocateInfo.descriptorPool     = pBindlessManager->m_DescriptorPool;

    Result = vkAllocateDescriptorSets(pDevice->GetDevice(), &DescriptorSetAllocateInfo, &pBindlessManager->m_DescriptorSet);
    if (Result != VK_SUCCESS)
    {
        LOG("Bindless vkAllocateDescriptorSets failed\n");
        SAFE_DELETE(pBindlessManager);
        return nullptr;
    }

    return pBindlessManager;
}

CDescriptorSetBindlessManager::CDescriptorSetBindlessManager(CDevice* pDevice)
    : IBindlessManager(pDevice)
    , m_DescriptorSet(VK_NULL_HANDLE)
    , m_DescriptorPool(VK_NULL_HANDLE)
    , m_DescriptorSetLayout(VK_NULL_HANDLE)
    , m_NextTextureBinding(0)
    , m_MaxTextureBinding(0)
{
}

CDescriptorSetBindlessManager::~CDescriptorSetBindlessManager()
{
    if (m_DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(GetDevice()->GetDevice(), m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }

    if (m_DescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(GetDevice()->GetDevice(), m_DescriptorSetLayout, nullptr);
        m_DescriptorSetLayout = VK_NULL_HANDLE;
    }
}

uint32_t CDescriptorSetBindlessManager::AddImageView(VkImageView ImageView, VkSampler Sampler)
{
    assert(m_DescriptorSet != VK_NULL_HANDLE);
    assert(Sampler != VK_NULL_HANDLE);
    assert(ImageView != VK_NULL_HANDLE);

    uint32_t BindlessHandle = InvalidBindlessID;
    if (m_TextureFreeList.empty())
    {
        if (m_NextTextureBinding < m_MaxTextureBinding)
        {
            BindlessHandle = m_NextTextureBinding++;
        }
    }
    else
    {
        BindlessHandle = m_TextureFreeList.back();
        m_TextureFreeList.pop_back();
    }

    if (BindlessHandle == InvalidBindlessID)
    {
        return InvalidBindlessID;
    }

    m_TextureBindings.insert(std::make_pair(ImageView, BindlessHandle));

    VkDescriptorImageInfo ImageInfo = {};
    ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ImageInfo.imageView   = ImageView;
    ImageInfo.sampler     = Sampler;

    VkWriteDescriptorSet DescriptorWrite = {};
    DescriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DescriptorWrite.dstSet          = m_DescriptorSet;
    DescriptorWrite.dstBinding      = BindlessSampledTextureBinding;
    DescriptorWrite.dstArrayElement = BindlessHandle;
    DescriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    DescriptorWrite.descriptorCount = 1;
    DescriptorWrite.pImageInfo      = &ImageInfo;

    vkUpdateDescriptorSets(GetDevice()->GetDevice(), 1, &DescriptorWrite, 0, nullptr);
    return BindlessHandle;
}

void CDescriptorSetBindlessManager::RemoveImageView(VkImageView ImageView)
{
    auto It = m_TextureBindings.find(ImageView);
    if (It == m_TextureBindings.end())
    {
        return;
    }

    m_TextureFreeList.emplace_back(It->second);
    m_TextureBindings.erase(It);
}

void CDescriptorSetBindlessManager::BindAccelerationStructure(VkAccelerationStructureKHR AccelerationStructure, uint32_t Binding)
{
    assert(m_DescriptorSet != VK_NULL_HANDLE);
    assert(Binding == BindlessAccelerationStructureBinding);
    assert(AccelerationStructure != VK_NULL_HANDLE);

    VkWriteDescriptorSetAccelerationStructureKHR AccelerationStructureInfo = {};
    AccelerationStructureInfo.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    AccelerationStructureInfo.accelerationStructureCount = 1;
    AccelerationStructureInfo.pAccelerationStructures    = &AccelerationStructure;

    VkWriteDescriptorSet DescriptorWrite = {};
    DescriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DescriptorWrite.pNext           = &AccelerationStructureInfo;
    DescriptorWrite.dstSet          = m_DescriptorSet;
    DescriptorWrite.dstBinding      = Binding;
    DescriptorWrite.dstArrayElement = 0;
    DescriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    DescriptorWrite.descriptorCount = 1;

    vkUpdateDescriptorSets(GetDevice()->GetDevice(), 1, &DescriptorWrite, 0, nullptr);
}

void CDescriptorSetBindlessManager::BindStorageImage(VkImageView ImageView, uint32_t Binding)
{
    assert(m_DescriptorSet != VK_NULL_HANDLE);
    assert(Binding == BindlessStorageOutputImageBinding || Binding == BindlessStoragePreviousImageBinding);
    assert(ImageView != VK_NULL_HANDLE);

    VkDescriptorImageInfo ImageInfo = {};
    ImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    ImageInfo.imageView   = ImageView;

    VkWriteDescriptorSet DescriptorWrite = {};
    DescriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DescriptorWrite.dstSet          = m_DescriptorSet;
    DescriptorWrite.dstBinding      = Binding;
    DescriptorWrite.dstArrayElement = 0;
    DescriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    DescriptorWrite.descriptorCount = 1;
    DescriptorWrite.pImageInfo      = &ImageInfo;

    vkUpdateDescriptorSets(GetDevice()->GetDevice(), 1, &DescriptorWrite, 0, nullptr);
}

void CDescriptorSetBindlessManager::BindCombinedImageSampler(VkImageView ImageView, VkSampler Sampler, uint32_t Binding)
{
    assert(m_DescriptorSet != VK_NULL_HANDLE);
    assert(Binding == BindlessSkyboxBinding);
    assert(ImageView != VK_NULL_HANDLE);
    assert(Sampler != VK_NULL_HANDLE);

    VkDescriptorImageInfo ImageInfo = {};
    ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ImageInfo.imageView   = ImageView;
    ImageInfo.sampler     = Sampler;

    VkWriteDescriptorSet DescriptorWrite = {};
    DescriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DescriptorWrite.dstSet          = m_DescriptorSet;
    DescriptorWrite.dstBinding      = Binding;
    DescriptorWrite.dstArrayElement = 0;
    DescriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    DescriptorWrite.descriptorCount = 1;
    DescriptorWrite.pImageInfo      = &ImageInfo;

    vkUpdateDescriptorSets(GetDevice()->GetDevice(), 1, &DescriptorWrite, 0, nullptr);
}

void CDescriptorSetBindlessManager::BindStorageBuffer(VkBuffer Buffer, VkDeviceSize Range, uint32_t Binding)
{
    assert(m_DescriptorSet != VK_NULL_HANDLE);
    assert((Binding >= BindlessStorageBufferBeginBinding && Binding <= BindlessStorageBufferEndBinding) || Binding == BindlessStorageTLASBinding);
    assert(Buffer != VK_NULL_HANDLE);
    assert(Range > 0);

    VkDescriptorBufferInfo BufferInfo = {};
    BufferInfo.buffer = Buffer;
    BufferInfo.offset = 0;
    BufferInfo.range  = Range;

    VkWriteDescriptorSet DescriptorWrite = {};
    DescriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DescriptorWrite.dstSet          = m_DescriptorSet;
    DescriptorWrite.dstBinding      = Binding;
    DescriptorWrite.dstArrayElement = 0;
    DescriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    DescriptorWrite.descriptorCount = 1;
    DescriptorWrite.pBufferInfo     = &BufferInfo;

    vkUpdateDescriptorSets(GetDevice()->GetDevice(), 1, &DescriptorWrite, 0, nullptr);
}

void CDescriptorSetBindlessManager::BindUniformBuffer(VkBuffer Buffer, VkDeviceSize Range, uint32_t Binding)
{
    assert(m_DescriptorSet != VK_NULL_HANDLE);
    assert(Binding == BindlessUniformCameraBinding || Binding == BindlessUniformRandomBinding || Binding == BindlessUniformSceneBinding);
    assert(Buffer != VK_NULL_HANDLE);
    assert(Range > 0);

    VkDescriptorBufferInfo BufferInfo = {};
    BufferInfo.buffer = Buffer;
    BufferInfo.offset = 0;
    BufferInfo.range  = Range;

    VkWriteDescriptorSet DescriptorWrite = {};
    DescriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DescriptorWrite.dstSet          = m_DescriptorSet;
    DescriptorWrite.dstBinding      = Binding;
    DescriptorWrite.dstArrayElement = 0;
    DescriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    DescriptorWrite.descriptorCount = 1;
    DescriptorWrite.pBufferInfo     = &BufferInfo;

    vkUpdateDescriptorSets(GetDevice()->GetDevice(), 1, &DescriptorWrite, 0, nullptr);
}

void CDescriptorSetBindlessManager::BindToCommandBuffer(CCommandBuffer* pCommandBuffer, CPipelineLayout* pPipelineLayout, VkPipelineBindPoint BindPoint)
{
    assert(pCommandBuffer != nullptr);
    assert(pPipelineLayout != nullptr);

    VkDescriptorSet DescriptorSet = m_DescriptorSet;
    vkCmdBindDescriptorSets(pCommandBuffer->GetCommandBuffer(), BindPoint, pPipelineLayout->GetPipelineLayout(), pPipelineLayout->GetBindlessDescriptorSetIndex(), 1, &DescriptorSet, 0, nullptr);
}

CDescriptorBufferBindlessManager* CDescriptorBufferBindlessManager::Create(CDevice* pDevice, uint32_t MaxTextureBinding)
{
    CDescriptorBufferBindlessManager* pBindlessManager = new CDescriptorBufferBindlessManager(pDevice);
    pBindlessManager->m_MaxTextureBinding = MaxTextureBinding;

    if (!CreateBindlessDescriptorSetLayout(pDevice, MaxTextureBinding, true, pBindlessManager->m_DescriptorSetLayout))
    {
        SAFE_DELETE(pBindlessManager);
        return nullptr;
    }

    Extensions::vkGetDescriptorSetLayoutSizeEXT(pDevice->GetDevice(), pBindlessManager->m_DescriptorSetLayout, &pBindlessManager->m_DescriptorBufferSize);
    pBindlessManager->m_DescriptorBufferBindingOffsets.fill(0);
    for (uint32_t Binding = BindlessSampledTextureBinding; Binding <= BindlessMaxBinding; Binding++)
    {
        if ((Binding == BindlessAccelerationStructureBinding) && !pDevice->IsRayTracingSupported())
        {
            continue;
        }

        Extensions::vkGetDescriptorSetLayoutBindingOffsetEXT(
            pDevice->GetDevice(),
            pBindlessManager->m_DescriptorSetLayout,
            Binding,
            &pBindlessManager->m_DescriptorBufferBindingOffsets[Binding]);
    }
    
    pBindlessManager->m_DescriptorBufferCombinedImageSamplerSize = pDevice->GetDescriptorBufferProperties().combinedImageSamplerDescriptorSize;
    pBindlessManager->m_DescriptorBufferStorageImageSize         = pDevice->GetDescriptorBufferProperties().storageImageDescriptorSize;
    pBindlessManager->m_DescriptorBufferStorageBufferSize        = pDevice->GetDescriptorBufferProperties().storageBufferDescriptorSize;
    pBindlessManager->m_DescriptorBufferUniformBufferSize        = pDevice->GetDescriptorBufferProperties().uniformBufferDescriptorSize;
    pBindlessManager->m_DescriptorBufferAccelerationStructureSize= pDevice->GetDescriptorBufferProperties().accelerationStructureDescriptorSize;
    pBindlessManager->m_DescriptorBufferUsageFlags               = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT;
    pBindlessManager->m_DescriptorBufferSetOffset                = 0;

    SBufferParams BufferParams = {};
    BufferParams.Size             = pBindlessManager->m_DescriptorBufferSize;
    BufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;
    BufferParams.Usage            =  VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    pBindlessManager->m_pDescriptorBuffer = CBuffer::Create(pDevice, BufferParams, nullptr);
    if (!pBindlessManager->m_pDescriptorBuffer)
    {
        SAFE_DELETE(pBindlessManager);
        return nullptr;
    }

    pBindlessManager->m_pMappedDescriptorBuffer = reinterpret_cast<uint8_t*>(pBindlessManager->m_pDescriptorBuffer->Map());
    if (!pBindlessManager->m_pMappedDescriptorBuffer)
    {
        SAFE_DELETE(pBindlessManager);
        return nullptr;
    }

    return pBindlessManager;
}

CDescriptorBufferBindlessManager::CDescriptorBufferBindlessManager(CDevice* pDevice)
    : IBindlessManager(pDevice)
    , m_DescriptorSetLayout(VK_NULL_HANDLE)
    , m_pDescriptorBuffer(nullptr)
    , m_pMappedDescriptorBuffer(nullptr)
    , m_DescriptorBufferUsageFlags(0)
    , m_DescriptorBufferSize(0)
    , m_DescriptorBufferSetOffset(0)
    , m_DescriptorBufferBindingOffsets()
    , m_DescriptorBufferCombinedImageSamplerSize(0)
    , m_DescriptorBufferStorageImageSize(0)
    , m_DescriptorBufferStorageBufferSize(0)
    , m_DescriptorBufferUniformBufferSize(0)
    , m_DescriptorBufferAccelerationStructureSize(0)
    , m_NextTextureBinding(0)
    , m_MaxTextureBinding(0)
{
    m_DescriptorBufferBindingOffsets.fill(0);
}

CDescriptorBufferBindlessManager::~CDescriptorBufferBindlessManager()
{
    if (m_pDescriptorBuffer)
    {
        if (m_pMappedDescriptorBuffer)
        {
            m_pDescriptorBuffer->Unmap();
            m_pMappedDescriptorBuffer = nullptr;
        }

        SAFE_DELETE(m_pDescriptorBuffer);
    }

    if (m_DescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(GetDevice()->GetDevice(), m_DescriptorSetLayout, nullptr);
        m_DescriptorSetLayout = VK_NULL_HANDLE;
    }
}

uint32_t CDescriptorBufferBindlessManager::AddImageView(VkImageView ImageView, VkSampler Sampler)
{
    assert(m_pMappedDescriptorBuffer != nullptr);
    assert(Sampler != VK_NULL_HANDLE);
    assert(ImageView != VK_NULL_HANDLE);

    uint32_t BindlessHandle = InvalidBindlessID;
    if (m_TextureFreeList.empty())
    {
        if (m_NextTextureBinding < m_MaxTextureBinding)
        {
            BindlessHandle = m_NextTextureBinding++;
        }
    }
    else
    {
        BindlessHandle = m_TextureFreeList.back();
        m_TextureFreeList.pop_back();
    }

    if (BindlessHandle == InvalidBindlessID)
    {
        return InvalidBindlessID;
    }

    m_TextureBindings.insert(std::make_pair(ImageView, BindlessHandle));

    VkDescriptorImageInfo ImageInfo = {};
    ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ImageInfo.imageView   = ImageView;
    ImageInfo.sampler     = Sampler;

    const VkDeviceSize DescriptorOffset = m_DescriptorBufferBindingOffsets[BindlessSampledTextureBinding] + VkDeviceSize(BindlessHandle) * m_DescriptorBufferCombinedImageSamplerSize;
    assert(DescriptorOffset + m_DescriptorBufferCombinedImageSamplerSize <= m_DescriptorBufferSize);

    VkDescriptorGetInfoEXT DescriptorGetInfo = {};
    DescriptorGetInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT;
    DescriptorGetInfo.type                       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    DescriptorGetInfo.data.pCombinedImageSampler = &ImageInfo;

    Extensions::vkGetDescriptorEXT(GetDevice()->GetDevice(), &DescriptorGetInfo, m_DescriptorBufferCombinedImageSamplerSize, m_pMappedDescriptorBuffer + DescriptorOffset);
    return BindlessHandle;
}

void CDescriptorBufferBindlessManager::RemoveImageView(VkImageView ImageView)
{
    auto It = m_TextureBindings.find(ImageView);
    if (It == m_TextureBindings.end())
    {
        return;
    }

    const VkDeviceSize DescriptorOffset = m_DescriptorBufferBindingOffsets[BindlessSampledTextureBinding] + VkDeviceSize(It->second) * m_DescriptorBufferCombinedImageSamplerSize;
    assert(DescriptorOffset + m_DescriptorBufferCombinedImageSamplerSize <= m_DescriptorBufferSize);
    memset(m_pMappedDescriptorBuffer + DescriptorOffset, 0, m_DescriptorBufferCombinedImageSamplerSize);

    m_TextureFreeList.emplace_back(It->second);
    m_TextureBindings.erase(It);
}

void CDescriptorBufferBindlessManager::BindAccelerationStructure(VkAccelerationStructureKHR AccelerationStructure, uint32_t Binding)
{
    assert(m_pMappedDescriptorBuffer != nullptr);
    assert(Binding == BindlessAccelerationStructureBinding);
    assert(AccelerationStructure != VK_NULL_HANDLE);
    assert(Extensions::vkGetAccelerationStructureDeviceAddressKHR != nullptr);

    VkAccelerationStructureDeviceAddressInfoKHR AddressInfo = {};
    AddressInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    AddressInfo.accelerationStructure = AccelerationStructure;

    const VkDeviceAddress AccelerationStructureAddress = Extensions::vkGetAccelerationStructureDeviceAddressKHR(GetDevice()->GetDevice(), &AddressInfo);
    const VkDeviceSize DescriptorOffset = m_DescriptorBufferBindingOffsets[Binding];
    assert(DescriptorOffset + m_DescriptorBufferAccelerationStructureSize <= m_DescriptorBufferSize);

    VkDescriptorGetInfoEXT DescriptorGetInfo = {};
    DescriptorGetInfo.sType                    = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT;
    DescriptorGetInfo.type                     = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    DescriptorGetInfo.data.accelerationStructure = AccelerationStructureAddress;

    Extensions::vkGetDescriptorEXT(GetDevice()->GetDevice(), &DescriptorGetInfo, m_DescriptorBufferAccelerationStructureSize, m_pMappedDescriptorBuffer + DescriptorOffset);
}

void CDescriptorBufferBindlessManager::BindStorageImage(VkImageView ImageView, uint32_t Binding)
{
    assert(m_pMappedDescriptorBuffer != nullptr);
    assert(Binding == BindlessStorageOutputImageBinding || Binding == BindlessStoragePreviousImageBinding);
    assert(ImageView != VK_NULL_HANDLE);

    VkDescriptorImageInfo ImageInfo = {};
    ImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    ImageInfo.imageView   = ImageView;

    const VkDeviceSize DescriptorOffset = m_DescriptorBufferBindingOffsets[Binding];
    assert(DescriptorOffset + m_DescriptorBufferStorageImageSize <= m_DescriptorBufferSize);

    VkDescriptorGetInfoEXT DescriptorGetInfo = {};
    DescriptorGetInfo.sType            = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT;
    DescriptorGetInfo.type             = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    DescriptorGetInfo.data.pStorageImage = &ImageInfo;

    Extensions::vkGetDescriptorEXT(GetDevice()->GetDevice(), &DescriptorGetInfo, m_DescriptorBufferStorageImageSize, m_pMappedDescriptorBuffer + DescriptorOffset);
}

void CDescriptorBufferBindlessManager::BindCombinedImageSampler(VkImageView ImageView, VkSampler Sampler, uint32_t Binding)
{
    assert(m_pMappedDescriptorBuffer != nullptr);
    assert(Binding == BindlessSkyboxBinding);
    assert(ImageView != VK_NULL_HANDLE);
    assert(Sampler != VK_NULL_HANDLE);

    VkDescriptorImageInfo ImageInfo = {};
    ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ImageInfo.imageView   = ImageView;
    ImageInfo.sampler     = Sampler;

    const VkDeviceSize DescriptorOffset = m_DescriptorBufferBindingOffsets[Binding];
    assert(DescriptorOffset + m_DescriptorBufferCombinedImageSamplerSize <= m_DescriptorBufferSize);

    VkDescriptorGetInfoEXT DescriptorGetInfo = {};
    DescriptorGetInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT;
    DescriptorGetInfo.type                       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    DescriptorGetInfo.data.pCombinedImageSampler = &ImageInfo;

    Extensions::vkGetDescriptorEXT(GetDevice()->GetDevice(), &DescriptorGetInfo, m_DescriptorBufferCombinedImageSamplerSize, m_pMappedDescriptorBuffer + DescriptorOffset);
}

void CDescriptorBufferBindlessManager::BindStorageBuffer(VkBuffer Buffer, VkDeviceSize Range, uint32_t Binding)
{
    assert(m_pMappedDescriptorBuffer != nullptr);
    assert((Binding >= BindlessStorageBufferBeginBinding && Binding <= BindlessStorageBufferEndBinding) || Binding == BindlessStorageTLASBinding);
    assert(Buffer != VK_NULL_HANDLE);
    assert(Range > 0);

    VkDescriptorAddressInfoEXT AddressInfo = {};
    AddressInfo.sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT;
    VkBufferDeviceAddressInfo BufferDeviceAddressInfo = {};
    BufferDeviceAddressInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    BufferDeviceAddressInfo.buffer = Buffer;
    AddressInfo.address = vkGetBufferDeviceAddress(GetDevice()->GetDevice(), &BufferDeviceAddressInfo);
    AddressInfo.range   = Range;
    AddressInfo.format  = VK_FORMAT_UNDEFINED;

    const VkDeviceSize DescriptorOffset = m_DescriptorBufferBindingOffsets[Binding];
    assert(DescriptorOffset + m_DescriptorBufferStorageBufferSize <= m_DescriptorBufferSize);

    VkDescriptorGetInfoEXT DescriptorGetInfo = {};
    DescriptorGetInfo.sType                  = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT;
    DescriptorGetInfo.type                   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    DescriptorGetInfo.data.pStorageBuffer    = &AddressInfo;

    Extensions::vkGetDescriptorEXT(GetDevice()->GetDevice(), &DescriptorGetInfo, m_DescriptorBufferStorageBufferSize, m_pMappedDescriptorBuffer + DescriptorOffset);
}

void CDescriptorBufferBindlessManager::BindUniformBuffer(VkBuffer Buffer, VkDeviceSize Range, uint32_t Binding)
{
    assert(m_pMappedDescriptorBuffer != nullptr);
    assert(Binding == BindlessUniformCameraBinding || Binding == BindlessUniformRandomBinding || Binding == BindlessUniformSceneBinding);
    assert(Buffer != VK_NULL_HANDLE);
    assert(Range > 0);

    VkDescriptorAddressInfoEXT AddressInfo = {};
    AddressInfo.sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT;
    VkBufferDeviceAddressInfo BufferDeviceAddressInfo = {};
    BufferDeviceAddressInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    BufferDeviceAddressInfo.buffer = Buffer;
    AddressInfo.address = vkGetBufferDeviceAddress(GetDevice()->GetDevice(), &BufferDeviceAddressInfo);
    AddressInfo.range   = Range;
    AddressInfo.format  = VK_FORMAT_UNDEFINED;

    const VkDeviceSize DescriptorOffset = m_DescriptorBufferBindingOffsets[Binding];
    assert(DescriptorOffset + m_DescriptorBufferUniformBufferSize <= m_DescriptorBufferSize);

    VkDescriptorGetInfoEXT DescriptorGetInfo = {};
    DescriptorGetInfo.sType                  = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT;
    DescriptorGetInfo.type                   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    DescriptorGetInfo.data.pUniformBuffer    = &AddressInfo;

    Extensions::vkGetDescriptorEXT(GetDevice()->GetDevice(), &DescriptorGetInfo, m_DescriptorBufferUniformBufferSize, m_pMappedDescriptorBuffer + DescriptorOffset);
}

void CDescriptorBufferBindlessManager::BindToCommandBuffer(CCommandBuffer* pCommandBuffer, CPipelineLayout* pPipelineLayout, VkPipelineBindPoint BindPoint)
{
    assert(pCommandBuffer != nullptr);
    assert(pPipelineLayout != nullptr);
    assert(Extensions::vkCmdBindDescriptorBuffersEXT != nullptr);
    assert(Extensions::vkCmdSetDescriptorBufferOffsetsEXT != nullptr);

    VkDescriptorBufferBindingInfoEXT BindingInfo = {};
    BindingInfo.sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
    BindingInfo.address = m_pDescriptorBuffer->GetDeviceAddress().deviceAddress;
    BindingInfo.usage   = m_DescriptorBufferUsageFlags;

    Extensions::vkCmdBindDescriptorBuffersEXT(pCommandBuffer->GetCommandBuffer(), 1, &BindingInfo);

    uint32_t     BufferIndex  = 0;
    VkDeviceSize BufferOffset = m_DescriptorBufferSetOffset;
    Extensions::vkCmdSetDescriptorBufferOffsetsEXT(pCommandBuffer->GetCommandBuffer(), BindPoint, pPipelineLayout->GetPipelineLayout(), pPipelineLayout->GetBindlessDescriptorSetIndex(), 1, &BufferIndex, &BufferOffset);
}
