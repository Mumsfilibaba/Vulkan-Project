#include "Scene.h"
#include "Model.h"
#include "Application.h"
#include "TextureResource.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/BindlessManager.h"

FScene::FScene(FDevice* pDevice)
    : IScene()
    , m_pDevice(pDevice)
    , m_Camera()
    , m_Settings()
    , m_pTopLevelAS(nullptr)
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
        for (const FSceneModel& ModelInstance : m_ModelInstances)
        {
            for (const FMaterial& Material : ModelInstance.Model->Materials)
            {
                if (Material.AlbedoTex)
                    pDevice->GetBindlessManager().RemoveImageView(Material.AlbedoTex->GetTextureView()->GetImageView());
                if (Material.NormalTex)
                    pDevice->GetBindlessManager().RemoveImageView(Material.NormalTex->GetTextureView()->GetImageView());
                if (Material.AlphaMaskTex)
                    pDevice->GetBindlessManager().RemoveImageView(Material.AlphaMaskTex->GetTextureView()->GetImageView());
                if (Material.RoughnessTex)
                    pDevice->GetBindlessManager().RemoveImageView(Material.RoughnessTex->GetTextureView()->GetImageView());
                if (Material.MetallicTex)
                    pDevice->GetBindlessManager().RemoveImageView(Material.MetallicTex->GetTextureView()->GetImageView());
            }
        }
    }
    else
    {
        // Return since we do not know if the device is still in use
        return;
    }

    SAFE_DELETE(m_pTopLevelAS);
    SAFE_DELETE(m_pMaterialSampler);
}

void FScene::Initialize()
{
    FDevice* pDevice = FApplication::Get().GetDevice();

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
    m_pMaterialSampler->SetDebugName("MaterialSampler");

    // Create a default material
    const size_t DefaultMaterialIndex = m_GpuMaterials.size();
    FMaterialGLSL& ShaderMaterial = m_GpuMaterials.emplace_back();
    ShaderMaterial.AlbedoColor           = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
    ShaderMaterial.EmissiveColor         = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    ShaderMaterial.SpecularColor         = glm::vec4(0.95f, 0.95f, 0.95f, 1.0f);
    ShaderMaterial.AbsorbtionColor       = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    ShaderMaterial.SpecularChance        = 0.1f;
    ShaderMaterial.SpecularRoughness     = 1.0f;
    ShaderMaterial.IncidenceOfRefraction = 1.0f;
    ShaderMaterial.RefractionChance      = 0.0f;
    ShaderMaterial.RefractionRoughness   = 0.0f;
    ShaderMaterial.AlbedoTexIndex        = FBindlessManager::InvalidBindlessID;
    ShaderMaterial.NormalTexIndex        = FBindlessManager::InvalidBindlessID;
    ShaderMaterial.AlphaMaskTexIndex     = FBindlessManager::InvalidBindlessID;
    ShaderMaterial.RoughnessTexIndex     = FBindlessManager::InvalidBindlessID;
    ShaderMaterial.MetallicTexIndex      = FBindlessManager::InvalidBindlessID;

    // Create geometries for the AccelerationStructure
    VkTransformMatrixKHR TransformMatrix =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f
    };

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

    FAccelerationStructureTLASParams TLASParams;
    for (const FSceneModel& ModelInstance : m_ModelInstances)
    {
        FTLASInstance& TLASInstance = TLASParams.Instances.emplace_back();
        TLASInstance.pBLAS           = ModelInstance.Model->pAccelerationStructure;
        TLASInstance.TransformMatrix = TransformMatrix;

        // Gather all materials from the model
        const size_t MaterialOffset = m_GpuMaterials.size();
        if (!ModelInstance.Model->Materials.empty())
        {
            for (const FMaterial& Material : ModelInstance.Model->Materials)
            {
                FMaterialGLSL& ShaderMaterial = m_GpuMaterials.emplace_back();
                ShaderMaterial.AlbedoColor           = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
                ShaderMaterial.EmissiveColor         = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
                ShaderMaterial.SpecularColor         = glm::vec4(0.95f, 0.95f, 0.95f, 1.0f);
                ShaderMaterial.AbsorbtionColor       = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
                ShaderMaterial.SpecularChance        = 0.1f;
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

        // Construct GPU mesh information
        for (const FModel::FSubMesh& SubMesh : ModelInstance.Model->SubMeshes)
        {
            FMeshInfo& MeshInfo = m_MeshInfoBuffer.emplace_back();
            MeshInfo.VertexBufferAddress = ModelInstance.Model->pVertexBuffer->GetDeviceAddress().deviceAddress;
            MeshInfo.IndexBufferAddress  = ModelInstance.Model->pIndexBuffer->GetDeviceAddress().deviceAddress;
            MeshInfo.IndexBufferAddress += SubMesh.IndexOffset * sizeof(uint32_t);

            // We need to offset the model's material-index into the global array of materials
            MeshInfo.MaterialIndex = (SubMesh.MaterialIndex >= 0) ? (MaterialOffset + SubMesh.MaterialIndex) : DefaultMaterialIndex;
        }
    }

    m_pTopLevelAS = FAccelerationStructure::CreateTLAS(pDevice, TLASParams);
    assert(m_pTopLevelAS != nullptr);
}

void FScene::Reset()
{
    m_Camera.Reset();
}

FScene* FSceneFactory::CreateScene(ESceneType SceneType)
{
    FDevice* pDevice = FApplication::Get().GetDevice();

    FScene* pScene = new FScene(pDevice);
    switch (SceneType)
    {
    case ESceneType::Spheres:
    case ESceneType::CornellBox:
    {
        std::shared_ptr<FModel> pSphereModel = std::make_shared<FModel>();
        pSphereModel->LoadFromFile(RESOURCE_PATH"/models/sphere.obj", pDevice);
        pScene->AddModel(pSphereModel);
        break;
    }

    case ESceneType::Triangles:
    {
        std::shared_ptr<FModel> pChessModel = std::make_shared<FModel>();
        pChessModel->LoadFromFile(RESOURCE_PATH"/models/queen.obj", pDevice);
        pScene->AddModel(pChessModel);
        break;
    }

    case ESceneType::Sponza:
    {
        std::shared_ptr<FModel> pSponzaModel = std::make_shared<FModel>();
        pSponzaModel->LoadFromFile(RESOURCE_PATH"/models/sponza/sponza.obj", pDevice);
        pScene->AddModel(pSponzaModel);
        pScene->m_Settings.CameraSpeed = 150.0f;
        break;
    }

    case ESceneType::PolishedGlassSpheres:
    case ESceneType::RoughColoredGlassSpheres:
    case ESceneType::RoughTransparentGlassSpheres:
    {
        std::shared_ptr<FModel> pSphereModel = std::make_shared<FModel>();
        pSphereModel->LoadFromFile(RESOURCE_PATH"/models/sphere.obj", pDevice);
        pScene->AddModel(pSphereModel);
        break;
    }

    default:
        break;
    }

    pScene->Initialize();
    return pScene;
}