#include "Scene.h"
#include "Model.h"
#include "Application.h"
#include "TextureResource.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/BindlessManager.h"

static constexpr uint32_t MESH_INFO_FLAG_SPHERICAL_NORMALS = 1u;

SScene::SScene(CDevice* pDevice)
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
    m_Settings.GradientLightStrength = 1.0f;
}

SScene::~SScene()
{
    if (CDevice* pDevice = CApplication::Get().GetDevice())
    {
        pDevice->WaitForIdle();

        // Cleanup any textures from the BindlessManager
        for (const SSceneModel& ModelInstance : m_ModelInstances)
        {
            for (const SMaterial& Material : ModelInstance.Model->Materials)
            {
                if (Material.AlbedoTex)
                {
                    pDevice->GetBindlessManager().RemoveImageView(Material.AlbedoTex->GetTextureView()->GetImageView());
                }

                if (Material.NormalTex)
                {
                    pDevice->GetBindlessManager().RemoveImageView(Material.NormalTex->GetTextureView()->GetImageView());
                }
                
                if (Material.AlphaMaskTex)
                {
                    pDevice->GetBindlessManager().RemoveImageView(Material.AlphaMaskTex->GetTextureView()->GetImageView());
                }
                
                if (Material.RoughnessTex)
                {
                    pDevice->GetBindlessManager().RemoveImageView(Material.RoughnessTex->GetTextureView()->GetImageView());
                }
                
                if (Material.MetallicTex)
                {
                    pDevice->GetBindlessManager().RemoveImageView(Material.MetallicTex->GetTextureView()->GetImageView());
                }
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

void SScene::Initialize()
{
    CDevice* pDevice = CApplication::Get().GetDevice();

    // Create Sampler for materials
    SSamplerParams SamplerParams = {};
    SamplerParams.MagFilter     = VK_FILTER_LINEAR;
    SamplerParams.MinFilter     = VK_FILTER_LINEAR;
    SamplerParams.MipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    SamplerParams.AddressModeU  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerParams.AddressModeV  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerParams.AddressModeW  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerParams.MinLod        = 0;
    SamplerParams.MaxLod        = 1000;
    SamplerParams.MaxAnisotropy = 1.0f;

    m_pMaterialSampler = CSampler::Create(pDevice, SamplerParams);
    assert(m_pMaterialSampler != nullptr);
    m_pMaterialSampler->SetDebugName("MaterialSampler");

    // Create a default material
    const size_t DefaultMaterialIndex = m_GpuMaterials.size();
    SMaterialHLSL& ShaderMaterial = m_GpuMaterials.emplace_back();
    ShaderMaterial.AlbedoColor           = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
    ShaderMaterial.EmissiveColor         = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    ShaderMaterial.SpecularColor         = glm::vec4(0.95f, 0.95f, 0.95f, 1.0f);
    ShaderMaterial.AbsorbtionColor       = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    ShaderMaterial.SpecularChance        = 0.1f;
    ShaderMaterial.SpecularRoughness     = 1.0f;
    ShaderMaterial.IncidenceOfRefraction = 1.0f;
    ShaderMaterial.RefractionChance      = 0.0f;
    ShaderMaterial.RefractionRoughness   = 0.0f;
    ShaderMaterial.AlbedoTexIndex        = CBindlessManager::InvalidBindlessID;
    ShaderMaterial.NormalTexIndex        = CBindlessManager::InvalidBindlessID;
    ShaderMaterial.AlphaMaskTexIndex     = CBindlessManager::InvalidBindlessID;
    ShaderMaterial.RoughnessTexIndex     = CBindlessManager::InvalidBindlessID;
    ShaderMaterial.MetallicTexIndex      = CBindlessManager::InvalidBindlessID;

    const auto AddImageViewToBindlessManager = [](const std::shared_ptr<CTextureResource>& Texture, CSampler* pSampler)
    {
        if (Texture)
        {
            CDevice* pDevice = CApplication::Get().GetDevice();
            return pDevice->GetBindlessManager().AddImageView(Texture->GetTextureView()->GetImageView(), pSampler->GetSampler());
        }
        else
        {
            return CBindlessManager::InvalidBindlessID;
        }
    };

    SAccelerationStructureTLASParams TLASParams;
    for (const SSceneModel& ModelInstance : m_ModelInstances)
    {
        // Translate
        glm::mat4 TransformMatrix = glm::identity<glm::mat4>();
        TransformMatrix = glm::translate(TransformMatrix, ModelInstance.Position);
        // Rotate
        glm::mat4 RotationMatrix = glm::yawPitchRoll(ModelInstance.Rotation.y, ModelInstance.Rotation.x, ModelInstance.Rotation.z);
        TransformMatrix = TransformMatrix * RotationMatrix;
        // Scale 
        TransformMatrix = glm::scale(TransformMatrix, ModelInstance.Scale);
        TransformMatrix = glm::transpose(TransformMatrix);

        VkTransformMatrixKHR TransformMatrixVk;
        memcpy(&TransformMatrixVk, glm::value_ptr(TransformMatrix), sizeof(VkTransformMatrixKHR));

        const size_t MeshInfoOffset = m_MeshInfoBuffer.size();
        STLASInstance& TLASInstance = TLASParams.Instances.emplace_back();
        TLASInstance.TransformMatrix     = TransformMatrixVk;
        TLASInstance.pBLAS               = ModelInstance.Model->pAccelerationStructure;
        TLASInstance.InstanceCustomIndex = MeshInfoOffset;
        TLASInstance.bDisableCulling     = ModelInstance.bDisableCulling;
        TLASInstance.bFlipTriangleFacing = ModelInstance.bFlipTriangleFacing;

        // Gather all materials from the model
        const size_t MaterialOffset = m_GpuMaterials.size();
        if (!ModelInstance.Model->Materials.empty())
        {
            for (const SMaterial& Material : ModelInstance.Model->Materials)
            {
                SMaterialHLSL& ShaderMaterial = m_GpuMaterials.emplace_back();
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
        for (const SModel::SSubMesh& SubMesh : ModelInstance.Model->SubMeshes)
        {
            SMeshInfo& MeshInfo = m_MeshInfoBuffer.emplace_back();
            MeshInfo.VertexBufferAddress = ModelInstance.Model->pVertexBuffer->GetDeviceAddress().deviceAddress;
            MeshInfo.IndexBufferAddress  = ModelInstance.Model->pIndexBuffer->GetDeviceAddress().deviceAddress;
            MeshInfo.IndexBufferAddress += SubMesh.IndexOffset * sizeof(uint32_t);
            MeshInfo.Flags               = 0;

            // We need to offset the model's material-index into the global array of materials
            if (ModelInstance.MaterialOverride >= 0)
            {
                MeshInfo.MaterialIndex = static_cast<uint32_t>(ModelInstance.MaterialOverride);
            }
            else
            {
                MeshInfo.MaterialIndex = (SubMesh.MaterialIndex >= 0) ? (MaterialOffset + SubMesh.MaterialIndex) : DefaultMaterialIndex;
            }
        }
    }

    m_pTopLevelAS = CAccelerationStructure::CreateTLAS(pDevice, TLASParams);
    assert(m_pTopLevelAS != nullptr);
}

void SScene::Reset()
{
    m_Camera.Reset();
}

SScene* SceneFactory::CreateScene(ESceneType SceneType)
{
    CDevice* pDevice = CApplication::Get().GetDevice();

    SScene* pScene = new SScene(pDevice);
    const auto AddMaterial = [pScene](const glm::vec4& AlbedoColor, const glm::vec4& EmissiveColor, const glm::vec4& SpecularColor, const glm::vec4& AbsorbtionColor, float SpecularChance, 
        float SpecularRoughness, float IncidenceOfRefraction, float RefractionChance, float RefractionRoughness)
    {
        SMaterialHLSL& Material = pScene->m_GpuMaterials.emplace_back();
        Material.AlbedoColor           = AlbedoColor;
        Material.EmissiveColor         = EmissiveColor;
        Material.SpecularColor         = SpecularColor;
        Material.AbsorbtionColor       = AbsorbtionColor;
        Material.SpecularChance        = SpecularChance;
        Material.SpecularRoughness     = SpecularRoughness;
        Material.IncidenceOfRefraction = IncidenceOfRefraction;
        Material.RefractionChance      = RefractionChance;
        Material.RefractionRoughness   = RefractionRoughness;
        Material.AlbedoTexIndex        = CBindlessManager::InvalidBindlessID;
        Material.NormalTexIndex        = CBindlessManager::InvalidBindlessID;
        Material.AlphaMaskTexIndex     = CBindlessManager::InvalidBindlessID;
        Material.RoughnessTexIndex     = CBindlessManager::InvalidBindlessID;
        Material.MetallicTexIndex      = CBindlessManager::InvalidBindlessID;
        return static_cast<int32_t>(pScene->m_GpuMaterials.size() - 1);
    };

    switch (SceneType)
    {
    case ESceneType::Spheres:
    {
        // Match software default sphere camera.
        pScene->m_Camera.Reset();
        pScene->m_Camera.Move(glm::vec3(0.0f, 1.0f, 0.75f));
        pScene->m_Camera.Rotate(glm::vec3(glm::pi<float>() / 4.0f, 0.0f, 0.0f));

        std::shared_ptr<SModel> pSphereModel = std::make_shared<SModel>();
        pSphereModel->LoadFromFile(RESOURCE_PATH"/models/sphere.obj", pDevice, true);

        const int32_t GoldenMaterial = AddMaterial(glm::vec4(0.8f, 0.6f, 0.2f, 1.0f), glm::vec4(0.0f), glm::vec4(0.8f, 0.6f, 0.2f, 1.0f), glm::vec4(0.0f), 0.9f, 0.5f, 1.0f, 0.0f, 0.0f);
        const int32_t PinkMaterial   = AddMaterial(glm::vec4(0.7f, 0.3f, 0.3f, 1.0f), glm::vec4(0.0f), glm::vec4(0.7f, 0.3f, 0.3f, 1.0f), glm::vec4(0.0f), 0.9f, 0.1f, 1.0f, 0.0f, 0.0f);
        const int32_t WhiteMaterial  = AddMaterial(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), glm::vec4(0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f), 0.0f, 0.6f, 1.0f, 0.0f, 0.0f);
        const int32_t GreenMaterial  = AddMaterial(glm::vec4(0.7f, 0.9f, 0.0f, 1.0f), glm::vec4(0.0f), glm::vec4(0.7f, 0.9f, 0.0f, 0.0f), glm::vec4(0.0f), 0.0f, 1.0f, 1.0f, 0.0f, 0.0f);

        pScene->AddModel(pSphereModel, glm::vec3( 1.0f, 0.0f, 1.0f), glm::vec3(0.5f), glm::vec3(0.0f), GoldenMaterial);
        pScene->AddModel(pSphereModel, glm::vec3( 0.0f, 0.0f, 1.0f), glm::vec3(0.5f), glm::vec3(0.0f), PinkMaterial);
        pScene->AddModel(pSphereModel, glm::vec3(-1.0f, 0.0f, 1.0f), glm::vec3(0.5f), glm::vec3(0.0f), WhiteMaterial);
        pScene->AddModel(pSphereModel, glm::vec3(0.0f, -100.5f, 0.0f), glm::vec3(100.0f), glm::vec3(0.0f), GreenMaterial);
        break;
    }

    case ESceneType::CornellBox:
    {
        // Match software Cornell camera.
        pScene->m_Camera.Reset();
        pScene->m_Camera.Move(glm::vec3(0.0f, 3.5f, 5.0f));
        pScene->m_Camera.Rotate(glm::vec3(glm::pi<float>() / 8.0f, glm::pi<float>(), 0.0f));

        std::shared_ptr<SModel> pSphereModel = std::make_shared<SModel>();
        pSphereModel->LoadFromFile(RESOURCE_PATH"/models/sphere.obj", pDevice, true);

        const int32_t WallMaterial    = AddMaterial(glm::vec4(0.7f, 0.7f, 0.7f, 1.0f), glm::vec4(0.0f), glm::vec4(0.0f), glm::vec4(0.0f), 0.0f, 0.9f, 1.0f, 0.0f, 0.0f);
        const int32_t RightMaterial   = AddMaterial(glm::vec4(0.7f, 0.1f, 0.1f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.0f), glm::vec4(0.0f), 0.0f, 0.9f, 1.0f, 0.0f, 0.0f);
        const int32_t LeftMaterial    = AddMaterial(glm::vec4(0.1f, 0.7f, 0.1f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.0f), glm::vec4(0.0f), 0.0f, 0.9f, 1.0f, 0.0f, 0.0f);
        const int32_t LightMaterial   = AddMaterial(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(20.0f, 18.0f, 14.0f, 1.0f), glm::vec4(0.0f), glm::vec4(0.0f), 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        const int32_t GreenMaterial0  = AddMaterial(glm::vec4(0.3f, 0.9f, 0.3f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.3f, 0.9f, 0.3f, 1.0f), glm::vec4(0.0f), 1.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        const int32_t GreenMaterial1  = AddMaterial(glm::vec4(0.3f, 0.9f, 0.3f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.3f, 0.9f, 0.3f, 1.0f), glm::vec4(0.0f), 1.0f, 0.25f, 1.0f, 0.0f, 0.0f);
        const int32_t GreenMaterial2  = AddMaterial(glm::vec4(0.3f, 0.9f, 0.3f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.3f, 0.9f, 0.3f, 1.0f), glm::vec4(0.0f), 1.0f, 0.5f, 1.0f, 0.0f, 0.0f);
        const int32_t GreenMaterial3  = AddMaterial(glm::vec4(0.3f, 0.9f, 0.3f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.3f, 0.9f, 0.3f, 1.0f), glm::vec4(0.0f), 1.0f, 0.75f, 1.0f, 0.0f, 0.0f);
        const int32_t GreenMaterial4  = AddMaterial(glm::vec4(0.3f, 0.9f, 0.3f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.3f, 0.9f, 0.3f, 1.0f), glm::vec4(0.0f), 1.0f, 1.0f, 1.0f, 0.0f, 0.0f);
        const int32_t BallMaterial0   = AddMaterial(glm::vec4(0.9f, 0.9f, 0.75f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.9f, 0.9f, 0.9f, 1.0f), glm::vec4(0.0f), 0.1f, 0.2f, 1.0f, 0.0f, 0.0f);
        const int32_t BallMaterial1   = AddMaterial(glm::vec4(0.9f, 0.75f, 0.9f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.9f, 0.9f, 0.9f, 1.0f), glm::vec4(0.0f), 0.5f, 0.2f, 1.0f, 0.0f, 0.0f);
        const int32_t BallMaterial2   = AddMaterial(glm::vec4(0.75f, 0.9f, 0.9f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.9f, 0.9f, 0.9f, 1.0f), glm::vec4(0.0f), 1.0f, 0.2f, 1.0f, 0.0f, 0.0f);

        pScene->AddModel(pSphereModel, glm::vec3( 2.0f, 2.5f, -1.5f), glm::vec3(0.25f), glm::vec3(0.0f), GreenMaterial0);
        pScene->AddModel(pSphereModel, glm::vec3( 1.0f, 2.5f, -1.5f), glm::vec3(0.25f), glm::vec3(0.0f), GreenMaterial1);
        pScene->AddModel(pSphereModel, glm::vec3( 0.0f, 2.5f, -1.5f), glm::vec3(0.25f), glm::vec3(0.0f), GreenMaterial2);
        pScene->AddModel(pSphereModel, glm::vec3(-1.0f, 2.5f, -1.5f), glm::vec3(0.25f), glm::vec3(0.0f), GreenMaterial3);
        pScene->AddModel(pSphereModel, glm::vec3(-2.0f, 2.5f, -1.5f), glm::vec3(0.25f), glm::vec3(0.0f), GreenMaterial4);

        pScene->AddModel(pSphereModel, glm::vec3( 2.2f, 0.75f, 0.5f), glm::vec3(0.7f), glm::vec3(0.0f), BallMaterial0);
        pScene->AddModel(pSphereModel, glm::vec3( 0.0f, 0.75f, 0.5f), glm::vec3(0.7f), glm::vec3(0.0f), BallMaterial1);
        pScene->AddModel(pSphereModel, glm::vec3(-2.2f, 0.75f, 0.5f), glm::vec3(0.7f), glm::vec3(0.0f), BallMaterial2);

        std::shared_ptr<SModel> pPlaneModel = std::make_shared<SModel>();
        pPlaneModel->LoadFromFile(RESOURCE_PATH"/models/plane.obj", pDevice);

        constexpr float PI      = glm::pi<float>();
        constexpr float HALF_PI = PI / 2.0f;

        pScene->AddModel(pPlaneModel, glm::vec3( 0.0f, 0.0f, 0.0f), glm::vec3(6.0f, 1.0f, 4.0f), glm::vec3(0.0f,    0.0f,     0.0f), WallMaterial);
        pScene->AddModel(pPlaneModel, glm::vec3( 0.0f, 2.0f, -2.0f), glm::vec3(6.0f, 1.0f, 4.0f), glm::vec3(HALF_PI, 0.0f,     0.0f), WallMaterial);
        pScene->AddModel(pPlaneModel, glm::vec3( 0.0f, 4.0f, 0.0f), glm::vec3(6.0f, 1.0f, 4.0f), glm::vec3(PI,      0.0f,     0.0f), WallMaterial);
        pScene->AddModel(pPlaneModel, glm::vec3(-3.0f, 2.0f, 0.0f), glm::vec3(4.0f, 1.0f, 4.0f), glm::vec3(0.0f,    0.0f, -HALF_PI), RightMaterial);
        pScene->AddModel(pPlaneModel, glm::vec3( 3.0f, 2.0f, 0.0f), glm::vec3(4.0f, 1.0f, 4.0f), glm::vec3(0.0f,    0.0f,  HALF_PI), LeftMaterial);
        pScene->AddModel(pPlaneModel, glm::vec3( 0.0f, 3.995f, 0.0f), glm::vec3(1.5f, 1.0f, 1.5f), glm::vec3(PI, 0.0f, 0.0f), LightMaterial);
        break;
    }

    case ESceneType::Triangles:
    {
        pScene->m_Camera.Reset();

        constexpr float PI      = glm::pi<float>();
        constexpr float HALF_PI = PI / 2.0f;

        pScene->m_Camera.Move(glm::vec3(0.0f, 0.5f, -0.75f));

        std::shared_ptr<SModel> pChessModel = std::make_shared<SModel>();
        pChessModel->LoadFromFile(RESOURCE_PATH"/models/queen.obj", pDevice);

        pScene->AddModel(pChessModel);

        std::shared_ptr<SModel> pPlaneModel = std::make_shared<SModel>();
        pPlaneModel->LoadFromFile(RESOURCE_PATH"/models/plane.obj", pDevice);

        const int32_t WallMaterial  = AddMaterial(glm::vec4(0.7f, 0.7f, 0.7f, 1.0f), glm::vec4(0.0f), glm::vec4(0.0f), glm::vec4(0.0f), 0.0f, 0.9f, 1.0f, 0.0f, 0.0f);
        const int32_t RightMaterial = AddMaterial(glm::vec4(0.7f, 0.1f, 0.1f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.0f), glm::vec4(0.0f), 0.0f, 0.9f, 1.0f, 0.0f, 0.0f);
        const int32_t LeftMaterial  = AddMaterial(glm::vec4(0.1f, 0.7f, 0.1f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.0f), glm::vec4(0.0f), 0.0f, 0.9f, 1.0f, 0.0f, 0.0f);
        const int32_t LightMaterial = AddMaterial(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), glm::vec4(20.0f, 20.0f, 20.0f, 1.0f), glm::vec4(0.0f), glm::vec4(0.0f), 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);

        pScene->AddModel(pPlaneModel, glm::vec3( 0.0f, 0.0f, 0.0f),  glm::vec3(2.0f, 1.0f, 2.0f), glm::vec3(0.0f,    0.0f,     0.0f), WallMaterial);
        pScene->AddModel(pPlaneModel, glm::vec3( 0.0f, 1.0f, 1.0f),  glm::vec3(2.0f, 1.0f, 2.0f), glm::vec3(HALF_PI, 0.0f,     0.0f), WallMaterial);
        pScene->AddModel(pPlaneModel, glm::vec3( 0.0f, 2.0f, 0.0f),  glm::vec3(2.0f, 1.0f, 2.0f), glm::vec3(PI,      0.0f,     0.0f), WallMaterial, false);
        pScene->AddModel(pPlaneModel, glm::vec3( 1.0f, 1.0f, 0.0f),  glm::vec3(2.0f, 1.0f, 2.0f), glm::vec3(0.0f,    0.0f, -HALF_PI), RightMaterial, false, true);
        pScene->AddModel(pPlaneModel, glm::vec3(-1.0f, 1.0f, 0.0f),  glm::vec3(2.0f, 1.0f, 2.0f), glm::vec3(0.0f,    0.0f,  HALF_PI), LeftMaterial, false, true);
        pScene->AddModel(pPlaneModel, glm::vec3(-0.7f, 1.95f, 0.0f), glm::vec3(0.4f, 1.0f, 0.4f), glm::vec3(PI,      0.0f,     0.0f), LightMaterial, false);
        break;
    }

    case ESceneType::Sponza:
    {
        // Match software model-scene camera/settings.
        pScene->m_Camera.Reset();
        pScene->m_Camera.Move(glm::vec3(0.0f, 0.5f, 1.75f));
        pScene->m_Camera.Rotate(glm::vec3(0.0f, glm::pi<float>(), 0.0f));

        std::shared_ptr<SModel> pSponzaModel = std::make_shared<SModel>();
        pSponzaModel->LoadFromFile(RESOURCE_PATH"/models/sponza/sponza.obj", pDevice);

        pScene->AddModel(pSponzaModel);
        pScene->m_Settings.CameraSpeed           = 150.0f;
        pScene->m_Settings.GradientLightStrength = 4.0f;
        break;
    }

    case ESceneType::PolishedGlassSpheres:
    case ESceneType::RoughColoredGlassSpheres:
    case ESceneType::RoughTransparentGlassSpheres:
    {
        // Match software glass-spheres camera.
        pScene->m_Camera.Reset();
        pScene->m_Camera.Move(glm::vec3(0.0f, 8.0f, 24.0f));
        pScene->m_Camera.Rotate(glm::vec3(0.0f, glm::pi<float>(), 0.0f));

        // Spheres
        std::shared_ptr<SModel> pSphereModel = std::make_shared<SModel>();
        pSphereModel->LoadFromFile(RESOURCE_PATH"/models/sphere.obj", pDevice, true);

        constexpr int32_t NumSpheres          = 7;
        constexpr float   SphereRadius        = 2.8f;
        constexpr float   SphereDiameter      = SphereRadius * 2.0f;
        constexpr float   SphereOffset        = 0.2f;
        constexpr float   SphereHalfFootPrint = SphereRadius + SphereOffset;
        constexpr float   SphereFootPrint     = SphereHalfFootPrint * 2.0f;
        constexpr float   Width               = SphereFootPrint * NumSpheres;
        constexpr float   HalfWidth           = Width / 2.0f;
        constexpr float   PI                  = glm::pi<float>();
        constexpr float   HALF_PI             = PI / 2.0f;

        const int32_t RoofFloorMaterial = AddMaterial(glm::vec4(0.9f, 0.9f, 0.9f, 1.0f), glm::vec4(0.0f), glm::vec4(0.0f), glm::vec4(0.0f), 0.0f, 0.9f, 1.0f, 0.0f, 0.0f);
        const int32_t WallMaterial      = AddMaterial(glm::vec4(0.02f, 0.02f, 0.02f, 1.0f), glm::vec4(0.0f), glm::vec4(0.0f), glm::vec4(0.0f), 0.0f, 0.9f, 1.0f, 0.0f, 0.0f);
        const int32_t LightMaterial     = AddMaterial(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(20.0f, 18.0f, 14.0f, 1.0f), glm::vec4(0.0f), glm::vec4(0.0f), 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);

        for (int32_t i = 0; i < NumSpheres; i++)
        {
            const float SphereStartPos = -(SphereHalfFootPrint - HalfWidth);
            int32_t SphereMaterial = -1;
            if (SceneType == ESceneType::PolishedGlassSpheres)
            {
                const float IncidenceOfRefraction = 1.0f + 0.5f * static_cast<float>(i) / static_cast<float>(NumSpheres - 1);
                SphereMaterial = AddMaterial(glm::vec4(0.9f, 0.25f, 0.25f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.8f, 0.8f, 0.8f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 0.02f, 0.0f, IncidenceOfRefraction, 1.0f, 0.0f);
            }
            else if (SceneType == ESceneType::RoughColoredGlassSpheres)
            {
                const float Roughness = static_cast<float>(i) / static_cast<float>(NumSpheres - 1) * 0.5f;
                SphereMaterial = AddMaterial(glm::vec4(0.9f, 0.25f, 0.25f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.8f, 0.8f, 0.8f, 1.0f), glm::vec4(0.0f, 0.5f, 1.0f, 1.0f), 0.02f, Roughness, 1.1f, 1.0f, Roughness);
            }
            else
            {
                const float Roughness = static_cast<float>(i) / static_cast<float>(NumSpheres - 1) * 0.5f;
                SphereMaterial = AddMaterial(glm::vec4(0.9f, 0.25f, 0.25f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.8f, 0.8f, 0.8f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 0.02f, Roughness, 1.1f, 1.0f, Roughness);
            }

            pScene->AddModel(pSphereModel, glm::vec3(SphereStartPos - (static_cast<float>(i) * SphereFootPrint), SphereRadius + SphereOffset, 0.0f), glm::vec3(SphereRadius), glm::vec3(0.0f), SphereMaterial);
        }

        // Quads
        std::shared_ptr<SModel> pPlaneModel = std::make_shared<SModel>();
        pPlaneModel->LoadFromFile(RESOURCE_PATH"/models/plane.obj", pDevice);

        // Roof Quad
        constexpr float RoofPos   = 23.0f;
        constexpr float RoofWidth = 15.0f;
        pScene->AddModel(pPlaneModel, glm::vec3(0.0f, RoofPos, 0.0f), glm::vec3(RoofWidth, 1.0f, RoofWidth), glm::vec3(PI, 0.0f, 0.0f), RoofFloorMaterial, false);

        // Light Quad
        constexpr float LightPos   = RoofPos - 0.1f;
        constexpr float LightWidth = 10.0f;
        pScene->AddModel(pPlaneModel, glm::vec3(0.0f, LightPos, 0.0f), glm::vec3(LightWidth, 1.0f, LightWidth), glm::vec3(PI, 0.0f, 0.0f), LightMaterial, false);

        // Floor Quad
        constexpr float FloorWidth     = Width + (SphereFootPrint * 2.0f);
        constexpr float FloorDepth     = SphereFootPrint + SphereRadius;
        constexpr float HalfFloorWidth = FloorWidth / 2.0f;
        pScene->AddModel(pPlaneModel, glm::vec3(0.0f, -2.0f, 0.0f), glm::vec3(FloorWidth, 1.0f, FloorDepth), glm::vec3(0.0f, 0.0f, 0.0f), RoofFloorMaterial, false);

        // Wall Quads
        constexpr uint32_t NumQuads = 100;

        constexpr float TotalWidth     = FloorWidth;
        constexpr float QuadWidth      = TotalWidth / NumQuads;
        constexpr float WallHeight     = 9.0f;
        constexpr float HalfWallHeight = WallHeight / 2.0f;

        for (uint32_t i = 0; i < NumQuads; i++)
        {
            const int32_t MaterialIndex = (i % 2 == 0) ? RoofFloorMaterial : WallMaterial;
            pScene->AddModel(pPlaneModel, glm::vec3(-HalfFloorWidth + (QuadWidth * static_cast<float>(i)), HalfWallHeight, -FloorDepth), glm::vec3(QuadWidth, 1.0f, WallHeight), glm::vec3(-HALF_PI, 0.0f, 0.0f), MaterialIndex, false, true);
        }

        pScene->m_Settings.CameraSpeed = 10.0f;
        break;
    }

    default:
        break;
    }

    pScene->Initialize();
    return pScene;
}