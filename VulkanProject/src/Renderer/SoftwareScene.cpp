#include "SoftwareScene.h"
#include "Model.h"
#include "Application.h"
#include "TextureResource.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/BindlessManager.h"
#include "Vulkan/Sampler.h"

template<typename T>
static void ZeroVector(std::vector<T>& OutVector)
{
    const size_t Size = sizeof(T) * OutVector.capacity();
    memset(OutVector.data(), 0, Size);
}

SSoftwareScene::SSoftwareScene()
    : m_Quads()
    , m_Spheres()
    , m_GpuMaterials()
    , m_Settings()
    , m_pVertexPositionsBuffer(nullptr)
    , m_pVertexBuffer(nullptr)
    , m_pIndexBuffer(nullptr)
    , m_pTriangleBuffer(nullptr)
    , m_pBoundingBoxBuffer(nullptr)
    , m_pMaterialSampler(nullptr)
    , m_pAABBInstanceBuffer(nullptr)
{
    m_Settings.ViewMode              = ESoftwareViewMode::Render;
    m_Settings.Exposure              = 0.5f;
    m_Settings.NumBounces            = 4;
    m_Settings.FieldOfView           = 90.0f;
    m_Settings.CameraSpeed           = 1.5f;
    m_Settings.GradientLightStrength = 1.0f;

    m_Quads.reserve(MAX_QUADS);
    ZeroVector(m_Quads);

    m_Spheres.reserve(MAX_SPHERES);
    ZeroVector(m_Spheres);

    m_GpuMaterials.reserve(MAX_MATERIALS);
    ZeroVector(m_GpuMaterials);

    m_VertexPositions.reserve(MAX_VERTICES);
    ZeroVector(m_VertexPositions);

    m_Vertices.reserve(MAX_VERTICES);
    ZeroVector(m_Vertices);

    m_Meshes.reserve(MAX_TRIANGLEMESHES);
    ZeroVector(m_Meshes);

    m_bUpdateBuffers = true;
}

SSoftwareScene::~SSoftwareScene()
{
    if (CDevice* pDevice = CApplication::Get().GetDevice())
    {
        pDevice->WaitForIdle();

        // Cleanup any textures from the BindlessManager
        for (const SMaterial& Material : m_Materials)
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

    SAFE_DELETE(m_pBoundingBoxBuffer);
    SAFE_DELETE(m_pTriangleBuffer);
    SAFE_DELETE(m_pVertexPositionsBuffer);
    SAFE_DELETE(m_pVertexBuffer);
    SAFE_DELETE(m_pIndexBuffer);
    SAFE_DELETE(m_pMaterialSampler);
    SAFE_DELETE(m_pAABBInstanceBuffer);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Triangle Model

void SModelScene::Initialize()
{
    // Settings
    m_Settings.BackgroundType = BACKGROUND_TYPE_GRADIENT;

    // Setup Camera
    Reset();

    if (Type == EModelSceneType::Default)
    {
        glm::vec3 Translation(0.0f, 0.0f, -1.0f);
        m_Camera.Move(Translation);
    }

    // Cache Device
    CDevice* pDevice = CApplication::Get().GetDevice();

    // Load Model
    SModel Model;
    if (Type == EModelSceneType::Default)
    {
        Model.LoadFromFile(RESOURCE_PATH"/models/queen.obj", pDevice);
        m_Settings.CameraSpeed = 1.5f;
    }
    else if (Type == EModelSceneType::Sponza)
    {
        Model.LoadFromFile(RESOURCE_PATH"/models/sponza/sponza.obj", pDevice);
        m_Settings.CameraSpeed           = 150.0f;
        m_Settings.GradientLightStrength = 4.0f;
    }

    // Copy data to the scene
    m_Vertices  = Model.Vertices;
    m_Materials = Model.Materials;

    // Create position only buffer
    m_VertexPositions.resize(m_Vertices.size());
    for (size_t i = 0; i < m_Vertices.size(); i++)
    {
        m_VertexPositions[i].Position = m_Vertices[i].Position;
    }

    // Build BVH
    m_AccelerationStructure.Build(Model, 32);

    // Copy data from the AccelerationStructure to the scene
    m_Indicies     = m_AccelerationStructure.m_Indicies;
    m_TriangleInfo = m_AccelerationStructure.m_TriangleInfo;

    LOG("Depth: %u\n", m_AccelerationStructure.Stats.Depth);
    LOG("MaxTrianglesInLeafNode: %u\n", m_AccelerationStructure.Stats.MaxTrianglesInLeafNode);
    LOG("Num BoundingBoxes: %u\n", m_AccelerationStructure.m_BoundingBoxes.size());

    // Mesh Data
    m_Meshes.push_back(
    {
        0, // BoundingBoxIndex
    });

    // BVH-Buffer
    SBufferParams BoundingBoxBufferParams;
    BoundingBoxBufferParams.Size             = sizeof(SShaderBoundingBox) * m_AccelerationStructure.m_BoundingBoxes.size();
    BoundingBoxBufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;
    BoundingBoxBufferParams.Usage            = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    assert(m_AccelerationStructure.m_BoundingBoxes.size() < MAX_BVH_NODES);
    m_pBoundingBoxBuffer = CBuffer::CreateWithData(pDevice, BoundingBoxBufferParams, nullptr, m_AccelerationStructure.m_BoundingBoxes.data());
    assert(m_pBoundingBoxBuffer != nullptr);
    m_pBoundingBoxBuffer->SetDebugName("CPU Bounding Box Buffer");

    // CPU Triangle Buffer
    SBufferParams TriangleBufferParams;
    TriangleBufferParams.Size             = sizeof(STriangleInfoHLSL) * m_TriangleInfo.size();
    TriangleBufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;
    TriangleBufferParams.Usage            = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pTriangleBuffer = CBuffer::CreateWithData(pDevice, TriangleBufferParams, nullptr, m_TriangleInfo.data());
    assert(m_pTriangleBuffer != nullptr);
    m_pTriangleBuffer->SetDebugName("CPU Triangle Buffer");

    // CPU VertexPositionsBuffer Buffer
    SBufferParams VertexPositionsBufferParams;
    VertexPositionsBufferParams.Size             = sizeof(SVertexPosition) * m_VertexPositions.size();
    VertexPositionsBufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;
    VertexPositionsBufferParams.Usage            = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pVertexPositionsBuffer = CBuffer::CreateWithData(pDevice, VertexPositionsBufferParams, nullptr, m_VertexPositions.data());
    assert(m_pVertexPositionsBuffer != nullptr);
    m_pVertexPositionsBuffer->SetDebugName("CPU VertexPositions Buffer");

    // CPU VertexBuffer Buffer
    SBufferParams VertexBufferParams;
    VertexBufferParams.Size             = sizeof(SVertex) * m_Vertices.size();
    VertexBufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;
    VertexBufferParams.Usage            = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pVertexBuffer = CBuffer::CreateWithData(pDevice, VertexBufferParams, nullptr, m_Vertices.data());
    assert(m_pVertexBuffer != nullptr);
    m_pVertexBuffer->SetDebugName("CPU Vertex Buffer");

    // CPU IndexBuffer Buffer
    SBufferParams IndexBufferParams;
    IndexBufferParams.Size             = sizeof(uint32_t) * m_Indicies.size();
    IndexBufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;
    IndexBufferParams.Usage            = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pIndexBuffer = CBuffer::CreateWithData(pDevice, IndexBufferParams, nullptr, m_Indicies.data());
    assert(m_pIndexBuffer != nullptr);
    m_pIndexBuffer->SetDebugName("CPU Index Buffer");

    // Create a matrix for each AABB
    std::vector<glm::mat4> AABBMatrices;
    AABBMatrices.reserve(m_AccelerationStructure.m_BoundingBoxes.size());

    for (size_t i = 0; i < m_AccelerationStructure.m_BoundingBoxes.size(); i++)
    {
        const SShaderBoundingBox& BoundingBox = m_AccelerationStructure.m_BoundingBoxes[i];
        if (BoundingBox.NumTriangles > 0)
        {
            glm::vec3 Scale    = glm::vec3(BoundingBox.BoxMax) - glm::vec3(BoundingBox.BoxMin);
            glm::vec3 Position = glm::vec3(BoundingBox.BoxMin) + (Scale * 0.5f);

            glm::mat4 TransformMatrix = glm::identity<glm::mat4>();
            TransformMatrix = glm::translate(TransformMatrix, Position);
            TransformMatrix = glm::scale(TransformMatrix, Scale);
            AABBMatrices.push_back(TransformMatrix);
        }
    }

    SBufferParams AABBInstanceBufferParams;
    AABBInstanceBufferParams.Size             = sizeof(glm::mat4) * AABBMatrices.size();
    AABBInstanceBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    AABBInstanceBufferParams.Usage            = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_pAABBInstanceBuffer = CBuffer::CreateWithData(pDevice, AABBInstanceBufferParams, nullptr, AABBMatrices.data());
    assert(m_pAABBInstanceBuffer != nullptr);
    m_pAABBInstanceBuffer->SetDebugName("CPU Debug AABB Instance Buffer");

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

    // Create ShaderMaterials for each materials
    const auto AddImageViewToBindlessManager = [](const std::shared_ptr<CTextureResource>& Texture, CSampler* pSampler)
    {
        if (Texture)
        {
            CDevice* pDevice = CApplication::Get().GetDevice();
            return pDevice->GetBindlessManager().AddImageView(Texture->GetTextureView()->GetImageView(), pSampler->GetSampler());
        }
        else
        {
            return IBindlessManager::InvalidBindlessID;
        }
    };

    for (const SMaterial& Material : m_Materials)
    {
        SMaterialHLSL& ShaderMaterial = m_GpuMaterials.emplace_back();
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

    // Sponza materials are non-emissive in the source MTL. Force-zero emissive to avoid
    // accidental lighting caused by fallback/index edge cases in the software path.
    if (Type == EModelSceneType::Sponza)
    {
        for (SMaterialHLSL& ShaderMaterial : m_GpuMaterials)
        {
            ShaderMaterial.EmissiveColor = glm::vec4(0.0f);
        }
    }

    const uint32_t WallMaterialIndex  = static_cast<uint32_t>(m_GpuMaterials.size());
    const uint32_t RightMaterialIndex = WallMaterialIndex + 1;
    const uint32_t LeftMaterialIndex  = WallMaterialIndex + 2;
    const uint32_t LightMaterialIndex = WallMaterialIndex + 3;

    // Quads
#if 1
    if (Type == EModelSceneType::Default)
    {
        // Floor Quad
        m_Quads.push_back({ glm::vec4(-1.0f, 0.0f, -1.0f, 0.0f), glm::vec4(0.0f, 0.0f, 2.0f, 0.0f), glm::vec4(2.0f, 0.0f, 0.0f, 0.0f), WallMaterialIndex });
        // Front Quad
        m_Quads.push_back({ glm::vec4(-1.0f, 2.0f, -1.0f, 0.0f), glm::vec4(0.0f, -2.0f, 0.0f, 0.0f), glm::vec4(2.0f, 0.0f, 0.0f, 0.0f), WallMaterialIndex });
        // Roof Quad
        m_Quads.push_back({ glm::vec4(-1.0f, 2.0f, 1.0f, 0.0f), glm::vec4(0.0f, 0.0f, -2.0f, 0.0f), glm::vec4(2.0f, 0.0f, 0.0f, 0.0f), WallMaterialIndex });
        // Right Quad
        m_Quads.push_back({ glm::vec4(-1.0f, 0.0f, -1.0f, 0.0f), glm::vec4(0.0f, 2.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 2.0f, 0.0f), RightMaterialIndex });
        // Left Quad
        m_Quads.push_back({ glm::vec4(1.0f, 2.0f, -1.0f, 0.0f), glm::vec4(0.0f, -2.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 2.0f, 0.0f), LeftMaterialIndex });
        // Light Quad
        m_Quads.push_back({ glm::vec4(0.5f, 1.95f, 0.2f, 0.0f), glm::vec4(0.0f, 0.0f, -0.4f, 0.0f), glm::vec4(0.4f, 0.0f, 0.0f, 0.0f), LightMaterialIndex });
    }
#endif

    if (Type == EModelSceneType::Default)
    {
        // Materials

        // Standard Wall material
        m_GpuMaterials.push_back(
        {
            glm::vec4(0.7f, 0.7f, 0.7f, 1.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            0.0f,
            0.9f,
            1.0f,
            0.0f,
            0.0f,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            // padding
            0, 0
        });

        // Right Wall Material
        m_GpuMaterials.push_back(
        {
            glm::vec4(0.7f, 0.1f, 0.1f, 1.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            0.0f,
            0.9f,
            1.0f,
            0.0f,
            0.0f,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            // padding
            0, 0
        });

        // Left Wall Material
        m_GpuMaterials.push_back(
        {
            glm::vec4(0.1f, 0.7f, 0.1f, 1.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            0.0f,
            0.9f,
            1.0f,
            0.0f,
            0.0f,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            // padding
            0, 0
        });

        // Emissive
        m_GpuMaterials.push_back(
        {
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            glm::vec4(40.0f, 40.0f, 40.0f, 1.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            // padding
            0, 0
        });
    }
}

void SModelScene::Reset()
{
    m_Camera.Reset();
    
    glm::vec3 Translation(0.0f, 0.5f, 1.75f);
    m_Camera.Move(Translation);

    glm::vec3 Rotation(0.0f, glm::pi<float>(), 0.0f);
    m_Camera.Rotate(Rotation);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Spheres

void SSphereScene::Initialize()
{
    // Setup Camera
    Reset();

    if (Type == ESphereSceneType::Default)
    {
        // Settings
        m_Settings.BackgroundType = BACKGROUND_TYPE_GRADIENT;

        // Spheres
        m_Spheres.push_back({ glm::vec3( 1.0f, 0.0f, 1.0f), 0.49f, 0 });
        m_Spheres.push_back({ glm::vec3(0.0f, 0.0f, 1.0f), 0.49f, 1 });
        m_Spheres.push_back({ glm::vec3(-1.0f, 0.0f, 1.0f), 0.49f, 2 });

        m_Spheres.push_back({ glm::vec3(0.0f, -100.5f, 0.0f), 100.0f, 3 });

        // Materials

        // Right Ball (Golden Ball)
        m_GpuMaterials.push_back(
        {
            glm::vec4(0.8f, 0.6f, 0.2f, 1.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.8f, 0.6f, 0.2f, 1.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            0.9f,
            0.5f,
            1.0f,
            0.0f,
            0.0f,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            // padding
            0, 0
        });

        // Middle Ball (Pink Ball)
        m_GpuMaterials.push_back(
        {
            glm::vec4(0.7f, 0.3f, 0.3f, 1.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.7f, 0.3f, 0.3f, 1.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            0.9f,
            0.1f,
            1.0f,
            0.0f,
            0.0f,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            // padding
            0, 0
        });

        // Left Ball (White Ball)
        m_GpuMaterials.push_back(
        {
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            0.0f,
            0.6f,
            1.0f,
            0.0f,
            0.0f,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            // padding
            0, 0
        });

        // Large Ball (Green Ball)
        m_GpuMaterials.push_back(
        {
            glm::vec4(0.7f, 0.9f, 0.0f, 1.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.7f, 0.9f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            0.0f,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            // padding
            0, 0
        });
    }
    else
    {
        constexpr int32_t NumSpheres        = 7;
        constexpr float SphereRadius        = 2.8f;
        constexpr float SphereDiameter      = SphereRadius * 2.0f;
        constexpr float SphereOffset        = 0.2f;
        constexpr float SphereHalfFootPrint = SphereRadius + SphereOffset;
        constexpr float SphereFootPrint     = SphereHalfFootPrint * 2.0f;
        constexpr float Width               = SphereFootPrint * NumSpheres;
        constexpr float HalfWidth           = Width / 2.0f;

        // Settings
        m_Settings.BackgroundType = BACKGROUND_TYPE_SKYBOX;

        // Roof Quad
        constexpr float RoofPos        = 23.0f;
        constexpr float RoofWidth      = 15.0f;
        constexpr float RoofHalfWidth  = RoofWidth / 2.0f;
        m_Quads.push_back({ glm::vec4(-RoofHalfWidth, RoofPos, RoofHalfWidth, 0.0f), glm::vec4(0.0f, 0.0f, -RoofWidth, 0.0f), glm::vec4(RoofWidth, 0.0f, 0.0f, 0.0f), 0 });

        // Light Quad
        constexpr float LightPos       = RoofPos - 0.1f;
        constexpr float LightWidth     = 10.0f;
        constexpr float LightHalfWidth = LightWidth / 2.0;
        m_Quads.push_back({ glm::vec4(-LightHalfWidth, LightPos, LightHalfWidth, 0.0f), glm::vec4(0.0f, 0.0f, -LightWidth, 0.0f), glm::vec4(LightWidth, 0.0f, 0.0f, 0.0f), 2 });

        // Floor Quad
        constexpr float FloorWidth     = Width + (SphereFootPrint * 2.0f);
        constexpr float FloorHalfWidth = FloorWidth / 2.0f;
        constexpr float FloorDepth     = SphereFootPrint + SphereRadius;
        constexpr float FloorHalfDepth = FloorDepth / 2.0f;
        m_Quads.push_back({ glm::vec4(-FloorHalfWidth, -2.0f, -FloorHalfDepth, 0.0f), glm::vec4(0.0f, 0.0f, FloorDepth, 0.0f), glm::vec4(FloorWidth, 0.0f, 0.0f, 0.0f), 0 });

        // Wall Quad
    #if 1
        constexpr uint32_t NumQuads = 100;
        constexpr float TotalWidth = FloorWidth;
        constexpr float QuadWidth  = TotalWidth / NumQuads;
        for (uint32_t i = 0; i < NumQuads; i++)
        {
            const uint32_t MaterialIndex = i % 2;
            m_Quads.push_back(
            {
                glm::vec4(-FloorHalfWidth + (QuadWidth * static_cast<float>(i)), 9.0f, -FloorDepth, 0.0f),
                glm::vec4(0.0f, -9.0, 0.0f, 0.0f),
                glm::vec4(QuadWidth, 0.0f, 0.0f, 0.0f),
                MaterialIndex
            });
        }
    #endif

        // Roof-, Floor- and Wall- Material
        m_GpuMaterials.push_back(
        {
            glm::vec4(0.9f, 0.9f, 0.9f, 1.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            0.0f,
            0.9f,
            1.0f,
            0.0f,
            0.0f,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            // padding
            0, 0
        });

        // Wall- Material
        m_GpuMaterials.push_back(
        {
            glm::vec4(0.02f, 0.02f, 0.02f, 1.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            0.0f,
            0.9f,
            1.0f,
            0.0f,
            0.0f,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            // padding
            0, 0
        });

        // Light Material
        m_GpuMaterials.push_back(
        {
            glm::vec4(0.0f,  0.0f,  0.0f, 1.0f),
            glm::vec4(20.0f, 18.0f, 14.0f, 1.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            IBindlessManager::InvalidBindlessID,
            // padding
            0, 0
        });

        // Spheres
        const uint32_t StartMaterialIndex = static_cast<uint32_t>(m_GpuMaterials.size());
        for (int32_t i = 0; i < NumSpheres; i++)
        {
            const float SphereStartPos = -(SphereHalfFootPrint - HalfWidth);
            m_Spheres.push_back(
            {
                glm::vec3(SphereStartPos - (static_cast<float>(i) * SphereFootPrint), SphereRadius + SphereOffset, 0.0f),
                SphereRadius,
                StartMaterialIndex + static_cast<uint32_t>(i)
            });
        }

        if (Type == ESphereSceneType::PolishedGlass)
        {
            for (int32_t i = 0; i < NumSpheres; i++)
            {
                const float IncidenceOfRefraction = 1.0f + 0.5f * float(i) / float(NumSpheres - 1);
                m_GpuMaterials.push_back(
                {
                    glm::vec4(0.9f, 0.25f, 0.25f, 1.0f),
                    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                    glm::vec4(0.8f, 0.8f, 0.8f, 1.0f),
                    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                    0.02f,
                    0.0f,
                    IncidenceOfRefraction,
                    1.0f,
                    0.0f,
                    IBindlessManager::InvalidBindlessID,
                    IBindlessManager::InvalidBindlessID,
                    IBindlessManager::InvalidBindlessID,
                    IBindlessManager::InvalidBindlessID,
                    IBindlessManager::InvalidBindlessID,
                    // padding
                    0, 0
                });
            }
        }
        else if (Type == ESphereSceneType::ColoredRoughGlass)
        {
            for (int32_t i = 0; i < NumSpheres; i++)
            {
                const float Roughness = float(i) / float(NumSpheres - 1) * 0.5f;
                m_GpuMaterials.push_back(
                {
                    glm::vec4(0.9f, 0.25f, 0.25f, 1.0f),
                    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                    glm::vec4(0.8f, 0.8f, 0.8f, 1.0f),
                    glm::vec4(0.0f, 0.5f, 1.0f, 1.0f),
                    0.02f,
                    Roughness,
                    1.1f,
                    1.0f,
                    Roughness,
                    IBindlessManager::InvalidBindlessID,
                    IBindlessManager::InvalidBindlessID,
                    IBindlessManager::InvalidBindlessID,
                    IBindlessManager::InvalidBindlessID,
                    IBindlessManager::InvalidBindlessID,
                    // padding
                    0, 0
                });
            }
        }
        else if (Type == ESphereSceneType::RoughGlass)
        {
            for (int32_t i = 0; i < NumSpheres; i++)
            {
                const float Roughness = float(i) / float(NumSpheres - 1) * 0.5f;
                m_GpuMaterials.push_back(
                {
                    glm::vec4(0.9f, 0.25f, 0.25f, 1.0f),
                    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                    glm::vec4(0.8f, 0.8f, 0.8f, 1.0f),
                    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                    0.02f,
                    Roughness,
                    1.1f,
                    1.0f,
                    Roughness,
                    IBindlessManager::InvalidBindlessID,
                    IBindlessManager::InvalidBindlessID,
                    IBindlessManager::InvalidBindlessID,
                    IBindlessManager::InvalidBindlessID,
                    IBindlessManager::InvalidBindlessID,
                    // padding
                    0, 0
                });
            }
        }
    }
}

void SSphereScene::Reset()
{
    m_Camera.Reset();

    if (Type == ESphereSceneType::Default)
    {
        glm::vec3 Translation(0.0f, 1.0f, 0.75f);
        m_Camera.Move(Translation);
        
        glm::vec3 Rotation(glm::pi<float>() / 4.0f, 0.0f, 0.0f);
        m_Camera.Rotate(Rotation);
    }
    else
    {
        m_Settings.CameraSpeed = 10.0f;
        
        glm::vec3 Translation(0.0f, 8.0f, 24.0f);
        m_Camera.Move(Translation);

        glm::vec3 Rotation(0.0f, glm::pi<float>(), 0.0f);
        m_Camera.Rotate(Rotation);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// CornellBox

void SCornellBoxScene::Initialize()
{
    // Settings
    m_Settings.BackgroundType = BACKGROUND_TYPE_GRADIENT;
    
    // Setup Camera
    Reset();

    // Quads
    
    // Floor Quad
    m_Quads.push_back({ glm::vec4(-3.0f, 0.0f, -2.0f, 0.0f), glm::vec4(0.0f, 0.0f, 4.0f, 0.0f), glm::vec4(6.0f, 0.0f, 0.0f, 0.0f), 0 });
    // Front Quad
    m_Quads.push_back({ glm::vec4(-3.0f, 4.0f, -2.0f, 0.0f), glm::vec4(0.0f, -4.0f, 0.0f, 0.0f), glm::vec4(6.0f, 0.0f, 0.0f, 0.0f), 0 });
    // Back Quad
#if 0 // NOTE: Disabled to let some light into the box for now
    m_Quads.push_back({ glm::vec4(-2.0f, 0.0f, 2.0f, 0.0f), glm::vec4(0.0f, 4.0f, 0.0f, 0.0f), glm::vec4(4.0f, 0.0f, 0.0f, 0.0f), 4 });
#endif
    // Roof Quad
    m_Quads.push_back({ glm::vec4(-3.0f, 4.0f, 2.0f, 0.0f), glm::vec4(0.0f, 0.0f, -4.0f, 0.0f), glm::vec4(6.0f, 0.0f, 0.0f, 0.0f), 0 });
    // Right Quad
    m_Quads.push_back({ glm::vec4(-3.0f, 0.0f, -2.0f, 0.0f), glm::vec4(0.0f, 4.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 4.0f, 0.0f), 1 });
    // Left Quad
    m_Quads.push_back({ glm::vec4(3.0f, 4.0f, -2.0f, 0.0f), glm::vec4(0.0f, -4.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 4.0f, 0.0f), 2 });
    // Light Quad
    m_Quads.push_back({ glm::vec4(-0.75f, 3.995f, 0.75f, 0.0f), glm::vec4(0.0f, 0.0f, -1.5f, 0.0f), glm::vec4(1.5f, 0.0f, 0.0f, 0.0f), 3 });

    // Spheres
    m_Spheres.push_back({ glm::vec3( 2.0f, 2.5f, -1.5f), 0.25f, 4 });
    m_Spheres.push_back({ glm::vec3( 1.0f, 2.5f, -1.5f), 0.25f, 5 });
    m_Spheres.push_back({ glm::vec3( 0.0f, 2.5f, -1.5f), 0.25f, 6 });
    m_Spheres.push_back({ glm::vec3(-1.0f, 2.5f, -1.5f), 0.25f, 7 });
    m_Spheres.push_back({ glm::vec3(-2.0f, 2.5f, -1.5f), 0.25f, 8 });
    
    m_Spheres.push_back({ glm::vec3( 2.2f, 0.75f, 0.5f), 0.7f, 9 });
    m_Spheres.push_back({ glm::vec3( 0.0f, 0.75f, 0.5f), 0.7f, 10 });
    m_Spheres.push_back({ glm::vec3(-2.2f, 0.75f, 0.5f), 0.7f, 11 });
    
    // Wall Materials
    m_GpuMaterials.push_back(
    {
        glm::vec4(0.7f, 0.7f, 0.7f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        0.0f,
        0.9f,
        1.0f,
        0.0f,
        0.0f,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        // padding
        0, 0
    });
    
    m_GpuMaterials.push_back(
    {
        glm::vec4(0.7f, 0.1f, 0.1f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        0.0f,
        0.9f,
        1.0f,
        0.0f,
        0.0f,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        // padding
        0, 0
    });
    
    m_GpuMaterials.push_back(
    {
        glm::vec4(0.1f, 0.7f, 0.1f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        0.0f,
        0.9f,
        1.0f,
        0.0f,
        0.0f,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        // padding
        0, 0
    });
    
    // Light
    m_GpuMaterials.push_back(
    {
        glm::vec4( 0.0f,  0.0f,  0.0f, 1.0f),
        glm::vec4(20.0f, 18.0f, 14.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        // padding
        0, 0
    });
    
    // Green Materials
    m_GpuMaterials.push_back(
    {
        glm::vec4(0.3f, 0.9f, 0.3f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.3f, 0.9f, 0.3f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        1.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        // padding
        0, 0
    });
    
    m_GpuMaterials.push_back(
    {
        glm::vec4(0.3f, 0.9f, 0.3f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.3f, 0.9f, 0.3f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        1.0f,
        0.25f,
        1.0f,
        0.0f,
        0.0f,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        // padding
        0, 0
    });
    
    m_GpuMaterials.push_back(
    {
        glm::vec4(0.3f, 0.9f, 0.3f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.3f, 0.9f, 0.3f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        1.0f,
        0.5f,
        1.0f,
        0.0f,
        0.0f,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        // padding
        0, 0
    });
    
    m_GpuMaterials.push_back(
    {
        glm::vec4(0.3f, 0.9f, 0.3f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.3f, 0.9f, 0.3f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        1.0f,
        0.75f,
        1.0f,
        0.0f,
        0.0f,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        // padding
        0, 0
    });
    
    m_GpuMaterials.push_back(
    {
        glm::vec4(0.3f, 0.9f, 0.3f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.3f, 0.9f, 0.3f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        1.0f,
        1.0f,
        1.0f,
        0.0f,
        0.0f,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        // padding
        0, 0
    });
    
    // Ball Materials
    m_GpuMaterials.push_back(
    {
        glm::vec4(0.9f, 0.9f, 0.75f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.9f, 0.9f, 0.9f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        0.1f,
        0.2f,
        1.0f,
        0.0f,
        0.0f,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        // padding
        0, 0
    });
    
    m_GpuMaterials.push_back(
    {
        glm::vec4(0.9f, 0.75f, 0.9f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.9f, 0.9f, 0.9f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        0.5f,
        0.2f,
        1.0f,
        0.0f,
        0.0f,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        // padding
        0, 0
    });
    
    m_GpuMaterials.push_back(
    {
        glm::vec4(0.75f, 0.9f, 0.9f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.9f, 0.9f, 0.9f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        1.0f,
        0.2f,
        1.0f,
        0.0f,
        0.0f,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        IBindlessManager::InvalidBindlessID,
        // padding
        0, 0
    });
}

void SCornellBoxScene::Reset()
{
    m_Camera.Reset();

    glm::vec3 Translation(0.0f, 3.5f, 5.0f);
    m_Camera.Move(Translation);

    glm::vec3 Rotation(glm::pi<float>() / 8.0f, glm::pi<float>(), 0.0f);
    m_Camera.Rotate(Rotation);
}
