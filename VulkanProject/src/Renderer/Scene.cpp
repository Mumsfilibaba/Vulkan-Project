#include "Scene.h"
#include "Model.h"
#include "Application.h"
#include "TextureResource.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/BindlessManager.h"

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
    m_Settings.GradientLightStrength = 4.0f;
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
    SMaterialGLSL& ShaderMaterial = m_GpuMaterials.emplace_back();
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

        // Gather all materials from the model
        const size_t MaterialOffset = m_GpuMaterials.size();
        if (!ModelInstance.Model->Materials.empty())
        {
            for (const SMaterial& Material : ModelInstance.Model->Materials)
            {
                SMaterialGLSL& ShaderMaterial = m_GpuMaterials.emplace_back();
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

            // We need to offset the model's material-index into the global array of materials
            MeshInfo.MaterialIndex = (SubMesh.MaterialIndex >= 0) ? (MaterialOffset + SubMesh.MaterialIndex) : DefaultMaterialIndex;
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
    switch (SceneType)
    {
    case ESceneType::Spheres:
    {
        std::shared_ptr<SModel> pSphereModel = std::make_shared<SModel>();
        pSphereModel->LoadFromFile(RESOURCE_PATH"/models/sphere.obj", pDevice);

        pScene->AddModel(pSphereModel, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.5f));
        pScene->AddModel(pSphereModel, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f));
        pScene->AddModel(pSphereModel, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.5f));
        pScene->AddModel(pSphereModel, glm::vec3(0.0f, -100.5f, 0.0f), glm::vec3(100.0f));
        break;
    }

    case ESceneType::CornellBox:
    {
        std::shared_ptr<SModel> pSphereModel = std::make_shared<SModel>();
        pSphereModel->LoadFromFile(RESOURCE_PATH"/models/sphere.obj", pDevice);

        pScene->AddModel(pSphereModel, glm::vec3( 2.0f, 2.5f, 1.5f), glm::vec3(0.25f));
        pScene->AddModel(pSphereModel, glm::vec3( 1.0f, 2.5f, 1.5f), glm::vec3(0.25f));
        pScene->AddModel(pSphereModel, glm::vec3( 0.0f, 2.5f, 1.5f), glm::vec3(0.25f));
        pScene->AddModel(pSphereModel, glm::vec3(-1.0f, 2.5f, 1.5f), glm::vec3(0.25f));
        pScene->AddModel(pSphereModel, glm::vec3(-2.0f, 2.5f, 1.5f), glm::vec3(0.25f));

        pScene->AddModel(pSphereModel, glm::vec3( 2.2f, 0.75f, -0.5f), glm::vec3(0.7f));
        pScene->AddModel(pSphereModel, glm::vec3( 0.0f, 0.75f, -0.5f), glm::vec3(0.7f));
        pScene->AddModel(pSphereModel, glm::vec3(-2.2f, 0.75f, -0.5f), glm::vec3(0.7f));

        std::shared_ptr<SModel> pPlaneModel = std::make_shared<SModel>();
        pPlaneModel->LoadFromFile(RESOURCE_PATH"/models/plane.obj", pDevice);

        constexpr float PI      = glm::pi<float>();
        constexpr float HALF_PI = PI / 2.0f;
        pScene->AddModel(pPlaneModel, glm::vec3( 0.0f, 0.0f, 0.0f), glm::vec3(6.0f, 1.0f, 4.0f), glm::vec3(0.0f,    0.0f,     0.0f));
        pScene->AddModel(pPlaneModel, glm::vec3( 0.0f, 2.0f, 2.0f), glm::vec3(6.0f, 1.0f, 4.0f), glm::vec3(HALF_PI, 0.0f,     0.0f));
        pScene->AddModel(pPlaneModel, glm::vec3( 0.0f, 4.0f, 0.0f), glm::vec3(6.0f, 1.0f, 4.0f), glm::vec3(PI,      0.0f,     0.0f));
        pScene->AddModel(pPlaneModel, glm::vec3( 3.0f, 2.0f, 0.0f), glm::vec3(4.0f, 1.0f, 4.0f), glm::vec3(0.0f,    0.0f, -HALF_PI));
        pScene->AddModel(pPlaneModel, glm::vec3(-3.0f, 2.0f, 0.0f), glm::vec3(4.0f, 1.0f, 4.0f), glm::vec3(0.0f,    0.0f,  HALF_PI));
        break;
    }

    case ESceneType::Triangles:
    {
        std::shared_ptr<SModel> pChessModel = std::make_shared<SModel>();
        pChessModel->LoadFromFile(RESOURCE_PATH"/models/queen.obj", pDevice);

        pScene->AddModel(pChessModel);

        std::shared_ptr<SModel> pPlaneModel = std::make_shared<SModel>();
        pPlaneModel->LoadFromFile(RESOURCE_PATH"/models/plane.obj", pDevice);

        constexpr float PI      = glm::pi<float>();
        constexpr float HALF_PI = PI / 2.0f;
        pScene->AddModel(pPlaneModel, glm::vec3( 0.0f, 0.0f, 0.0f), glm::vec3(2.0f, 1.0f, 2.0f), glm::vec3(0.0f,    0.0f,     0.0f));
        pScene->AddModel(pPlaneModel, glm::vec3( 0.0f, 1.0f, 1.0f), glm::vec3(2.0f, 1.0f, 2.0f), glm::vec3(HALF_PI, 0.0f,     0.0f));
        pScene->AddModel(pPlaneModel, glm::vec3( 0.0f, 2.0f, 0.0f), glm::vec3(2.0f, 1.0f, 2.0f), glm::vec3(PI,      0.0f,     0.0f));
        pScene->AddModel(pPlaneModel, glm::vec3( 1.0f, 1.0f, 0.0f), glm::vec3(2.0f, 1.0f, 2.0f), glm::vec3(0.0f,    0.0f, -HALF_PI));
        pScene->AddModel(pPlaneModel, glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(2.0f, 1.0f, 2.0f), glm::vec3(0.0f,    0.0f,  HALF_PI));
        break;
    }

    case ESceneType::Sponza:
    {
        std::shared_ptr<SModel> pSponzaModel = std::make_shared<SModel>();
        pSponzaModel->LoadFromFile(RESOURCE_PATH"/models/sponza/sponza.obj", pDevice);

        pScene->AddModel(pSponzaModel);
        pScene->m_Settings.CameraSpeed = 150.0f;
        break;
    }

    case ESceneType::PolishedGlassSpheres:
    case ESceneType::RoughColoredGlassSpheres:
    case ESceneType::RoughTransparentGlassSpheres:
    {
        // Spheres
        std::shared_ptr<SModel> pSphereModel = std::make_shared<SModel>();
        pSphereModel->LoadFromFile(RESOURCE_PATH"/models/sphere.obj", pDevice);

        constexpr int32_t NumSpheres        = 7;
        constexpr float SphereRadius        = 2.8f;
        constexpr float SphereDiameter      = SphereRadius * 2.0f;
        constexpr float SphereOffset        = 0.2f;
        constexpr float SphereHalfFootPrint = SphereRadius + SphereOffset;
        constexpr float SphereFootPrint     = SphereHalfFootPrint * 2.0f;
        constexpr float Width               = SphereFootPrint * NumSpheres;
        constexpr float HalfWidth           = Width / 2.0f;
        constexpr float PI                  = glm::pi<float>();
        constexpr float HALF_PI             = PI / 2.0f;

        for (int32_t i = 0; i < NumSpheres; i++)
        {
            const float SphereStartPos = -(SphereHalfFootPrint - HalfWidth);
            pScene->AddModel(pSphereModel, glm::vec3(SphereStartPos - (static_cast<float>(i) * SphereFootPrint), SphereRadius + SphereOffset, 0.0f), glm::vec3(SphereRadius));
        }

        // Quads
        std::shared_ptr<SModel> pPlaneModel = std::make_shared<SModel>();
        pPlaneModel->LoadFromFile(RESOURCE_PATH"/models/plane.obj", pDevice);

        // Roof Quad
        constexpr float RoofPos   = 23.0f;
        constexpr float RoofWidth = 15.0f;
        pScene->AddModel(pPlaneModel, glm::vec3(0.0f, RoofPos, 0.0f), glm::vec3(RoofWidth, 1.0f, RoofWidth), glm::vec3(PI, 0.0f, 0.0f));

        // Light Quad
        constexpr float LightPos   = RoofPos - 0.1f;
        constexpr float LightWidth = 10.0f;
        pScene->AddModel(pPlaneModel, glm::vec3(0.0f, LightPos, 0.0f), glm::vec3(LightWidth, 1.0f, LightWidth), glm::vec3(PI, 0.0f, 0.0f));

        // Floor Quad
        constexpr float FloorWidth     = Width + (SphereFootPrint * 2.0f);
        constexpr float FloorDepth     = SphereFootPrint + SphereRadius;
        constexpr float HalfFloorWidth = FloorWidth / 2.0f;
        pScene->AddModel(pPlaneModel, glm::vec3(0.0f, -2.0f, 0.0f), glm::vec3(FloorWidth, 1.0f, FloorDepth), glm::vec3(0.0f, 0.0f, 0.0f));

        // Wall Quads
        constexpr uint32_t NumQuads = 100;

        constexpr float TotalWidth     = FloorWidth;
        constexpr float QuadWidth      = TotalWidth / NumQuads;
        constexpr float WallHeight     = 9.0f;
        constexpr float HalfWallHeight = WallHeight / 2.0f;

        for (uint32_t i = 0; i < NumQuads; i++)
        {
            pScene->AddModel(pPlaneModel, glm::vec3(-HalfFloorWidth + (QuadWidth * static_cast<float>(i)), HalfWallHeight, -FloorDepth), glm::vec3(QuadWidth, 1.0f, WallHeight), glm::vec3(-HALF_PI, 0.0f, 0.0f));
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