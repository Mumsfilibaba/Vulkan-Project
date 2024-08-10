#include "Scene.h"
#include "Model.h"
#include "Application.h"
#include "TextureResource.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/BindlessManager.h"

#define SPONZA 1

FScene::FScene(FDevice* pDevice)
    : m_pDevice(pDevice)
    , m_Camera()
    , m_Settings()
    , m_VertexBuffers()
    , m_IndexBuffers()
    , m_pTopLevelAS(nullptr)
    , m_BottomLevelASs()
    , m_pMaterialSampler(nullptr)
{
    assert(pDevice != nullptr);

    m_Settings.ViewMode              = EViewMode::Render;
    m_Settings.BackgroundType        = EBackgroundType::Gradient;
    m_Settings.Exposure              = 0.5f;
    m_Settings.NumBounces            = 4;
    m_Settings.FieldOfView           = 90.0f;
    m_Settings.CameraSpeed           = 1.5f;
    m_Settings.GradientLightStrength = 4.0f;
}

FScene::~FScene()
{
    if (FDevice* pDevice = FApplication::Get().GetDevice())
    {
        pDevice->WaitForIdle();

        // Cleanup any textures from the BindlessManager
        for (const FMaterial& Material : m_Materials)
        {
            if (Material.AlbedoTex)
            {
                pDevice->GetBindlessManager().RemoveImageView(Material.AlbedoTex->GetTextureView()->GetImageView());
            }
            if (Material.NormalTex)
            {
                pDevice->GetBindlessManager().RemoveImageView(Material.NormalTex->GetTextureView()->GetImageView());
            }
        }
    }
    else
    {
        // Return since we do not know if the device is still in use
        return;
    }

    for (FBuffer* pBuffer : m_VertexBuffers)
    {
        SAFE_DELETE(pBuffer);
    }
    for (FBuffer* pBuffer : m_IndexBuffers)
    {
        SAFE_DELETE(pBuffer);
    }
    for (FAccelerationStructure* pAccelerationStructure : m_BottomLevelASs)
    {
        SAFE_DELETE(pAccelerationStructure);
    }

    SAFE_DELETE(m_pTopLevelAS);
    SAFE_DELETE(m_pMaterialSampler);
}

void FScene::Initialize()
{
    FDevice* pDevice = FApplication::Get().GetDevice();
    FModel* Model = new FModel();

#if SPONZA
    Model->LoadFromFile(RESOURCE_PATH"/models/sponza/sponza.obj", pDevice);
    m_Settings.CameraSpeed = 150.0f;
#else
    Model->LoadFromFile(RESOURCE_PATH"/models/queen.obj", pDevice);
    m_Settings.CameraSpeed = 1.5f;
#endif

    // Copy Materials
    m_Materials = Model->Materials;

    // Copy the buffers from the model
    FBufferParams VertexBufferParams = {};
    VertexBufferParams.Size             = Model->VertexCount * sizeof(FVertex);
    VertexBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_RAY_TRACING_INPUT;
    VertexBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;

    FBuffer* pVertexBuffer = FBuffer::CreateAndCopy(pDevice, VertexBufferParams, nullptr, Model->pVertexBuffer);
    assert(pVertexBuffer != nullptr);
    pVertexBuffer->SetDebugName("Scene VertexBuffer");

    FBufferParams IndexBufferParams = {};
    IndexBufferParams.Size             = Model->IndexCount * sizeof(uint32_t);
    IndexBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_RAY_TRACING_INPUT;
    IndexBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;

    FBuffer* pIndexBuffer = FBuffer::CreateAndCopy(pDevice, IndexBufferParams, nullptr, Model->pIndexBuffer);
    assert(pIndexBuffer != nullptr);
    pIndexBuffer->SetDebugName("Scene IndexBuffer");

    m_VertexBuffers.push_back(pVertexBuffer);
    m_IndexBuffers.push_back(pIndexBuffer);

    // Create geometries for the AccelerationStructure
    VkTransformMatrixKHR TransformMatrix =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f
    };

    // BLAS for opaque objects
    FAccelerationStructureBLASParams BLASParams;
    for (const FModel::FSubMesh& SubMesh : Model->SubMeshes)
    {
        const bool bHasAlphaMask = (SubMesh.MaterialIndex >= 0) ? (Model->Materials[SubMesh.MaterialIndex].AlphaMaskTex != nullptr) : false;

        FBLASGeometry& Geometry = BLASParams.Geometries.emplace_back();
        Geometry.Flags              = bHasAlphaMask ? 0 : VK_GEOMETRY_OPAQUE_BIT_KHR;
        Geometry.TransformMatrix    = TransformMatrix;
        Geometry.pVertexBuffer      = pVertexBuffer;
        Geometry.MaxVertexIndex     = Model->VertexCount;
        Geometry.VertexBufferCount  = SubMesh.VertexCount;
        Geometry.VertexBufferOffset = SubMesh.VertexOffset;
        Geometry.VertexStride       = sizeof(FVertex);
        Geometry.pIndexBuffer       = pIndexBuffer;
        Geometry.IndexBufferOffset  = SubMesh.IndexOffset;
        Geometry.IndexBufferCount   = SubMesh.IndexCount;

        FMeshInfo& MeshInfo = m_MeshInfoBuffer.emplace_back();
        MeshInfo.MaterialIndex       = SubMesh.MaterialIndex;
        MeshInfo.VertexBufferAddress = pVertexBuffer->GetDeviceAddress().deviceAddress;
        MeshInfo.IndexBufferAddress  = pIndexBuffer->GetDeviceAddress().deviceAddress;
        MeshInfo.IndexBufferAddress += Geometry.IndexBufferOffset * sizeof(uint32_t);
    }

    // Cleanup the old VertexBuffers
    SAFE_DELETE(Model);

    // Create Bottom-Level AccelerationStructure
    FAccelerationStructure* pBottomLevelAS = FAccelerationStructure::CreateBLAS(pDevice, BLASParams);
    assert(pBottomLevelAS != nullptr);
    m_BottomLevelASs.push_back(pBottomLevelAS);

    // Create Top-Level AccelerationStructure
    FAccelerationStructureTLASParams TLASParams;
    TLASParams.pAccelerationStructure = m_BottomLevelASs[0];

    m_pTopLevelAS = FAccelerationStructure::CreateTLAS(pDevice, TLASParams);
    assert(m_pTopLevelAS != nullptr);

    // Create Sampler for materials
    FSamplerParams SamplerParams = {};
    SamplerParams.MagFilter     = VK_FILTER_LINEAR;
    SamplerParams.MinFilter     = VK_FILTER_LINEAR;
    SamplerParams.MipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    SamplerParams.AddressModeU  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerParams.AddressModeV  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerParams.AddressModeW  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerParams.MinLod        = 0;
    SamplerParams.MaxLod        = 1000;
    SamplerParams.MaxAnisotropy = 1.0f;

    m_pMaterialSampler = FSampler::Create(pDevice, SamplerParams);
    assert(m_pMaterialSampler != nullptr);
    m_pMaterialSampler->SetDebugName("Material Sampler");

    // Create ShaderMaterials for each materials
    const auto AddImageViewToBindlessManager = [](const std::shared_ptr<FTextureResource>& Texture, FSampler* pSampler)
    {
        if (Texture)
        {
            FDevice* pDevice = FApplication::Get().GetDevice();
            return pDevice->GetBindlessManager().AddImageView(Texture->GetTextureView()->GetImageView(), pSampler->GetSampler());
        }
        else
        {
            return FBindlessManager::InvalidBindlessID;
        }
    };

    for (const FMaterial& Material : m_Materials)
    {
        FMaterialGLSL& ShaderMaterial = m_GpuMaterials.emplace_back();
        ShaderMaterial.AlbedoColor           = glm::vec4(0.95f, 0.95f, 0.95f, 1.0f);
        ShaderMaterial.EmissiveColor         = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
        ShaderMaterial.SpecularColor         = glm::vec4(0.95f, 0.95f, 0.95f, 1.0f);
        ShaderMaterial.AbsorbtionColor       = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
        ShaderMaterial.SpecularChance        = 0.0f;
        ShaderMaterial.SpecularRoughness     = 1.0f;
        ShaderMaterial.IncidenceOfRefraction = 1.0f;
        ShaderMaterial.RefractionChance      = 0.0f;
        ShaderMaterial.RefractionRoughness   = 0.0f;

        // Add texture to the BindlessManager if there is a texture for this material
        ShaderMaterial.AlbedoTexIndex    = AddImageViewToBindlessManager(Material.AlbedoTex, m_pMaterialSampler);
        ShaderMaterial.NormalTexIndex    = AddImageViewToBindlessManager(Material.NormalTex, m_pMaterialSampler);
        ShaderMaterial.AlphaMaskTexIndex = AddImageViewToBindlessManager(Material.AlphaMaskTex, m_pMaterialSampler);
        ShaderMaterial.RoughnessTexIndex = AddImageViewToBindlessManager(Material.RoughnessTex, m_pMaterialSampler);
        ShaderMaterial.MetallicTexIndex  = AddImageViewToBindlessManager(Material.MetallicTex, m_pMaterialSampler);
    }
}

void FScene::Reset()
{
    m_Camera.Reset();
}