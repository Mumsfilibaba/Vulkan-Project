#include "Scene.h"
#include "Model.h"
#include "Application.h"
#include "TextureResource.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/BindlessManager.h"
#include "Vulkan/Sampler.h"

FScene::FScene()
    : m_Quads()
    , m_Spheres()
    , m_GpuMaterials()
    , m_Settings()
    , m_pVertexBuffer(nullptr)
    , m_pVertexExBuffer(nullptr)
    , m_pTriangleBuffer(nullptr)
    , m_pBoundingBoxBuffer(nullptr)
    , m_pMaterialSampler(nullptr)
    , m_pMeshVertexBuffer(nullptr)
    , m_pMeshIndexBuffer(nullptr)
    , m_pAABBInstanceBuffer(nullptr)
{
    m_Settings.ViewMode              = EViewMode::Render;
    m_Settings.Exposure              = 0.5f;
    m_Settings.NumBounces            = 4;
    m_Settings.FieldOfView           = 90.0f;
    m_Settings.CameraSpeed           = 1.5f;
    m_Settings.GradientLightStrength = 1.0f;
    
    m_Quads.reserve(MAX_QUADS);
    m_Spheres.reserve(MAX_SPHERES);
    m_GpuMaterials.reserve(MAX_MATERIALS);
    m_Vertices.reserve(MAX_VERTICES);
    m_VerticesEx.reserve(MAX_VERTICES);
    m_Meshes.reserve(MAX_TRIANGLEMESHES);
    
    m_bUpdateBuffers = true;
}

FScene::~FScene()
{
    if (FDevice* pDevice = FApplication::Get().GetDevice())
    {
        pDevice->WaitForIdle();
        
        // Cleanup any textures from the BindlessManager
        for (const auto& Material : m_Materials)
        {
            if (Material.AlbedoTex)
            {
                pDevice->GetBindlessManager().RemoveImageView(Material.AlbedoTex->GetTextureView()->GetImageView());
            }
        }
    }
    
    SAFE_DELETE(m_pBoundingBoxBuffer);
    SAFE_DELETE(m_pTriangleBuffer);
    SAFE_DELETE(m_pVertexBuffer);
    SAFE_DELETE(m_pVertexExBuffer);
    SAFE_DELETE(m_pMaterialSampler);

    SAFE_DELETE(m_pMeshVertexBuffer);
    SAFE_DELETE(m_pMeshIndexBuffer);
    SAFE_DELETE(m_pAABBInstanceBuffer);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Triangle Model

void FModelScene::Initialize()
{
    // Settings
    m_Settings.BackgroundType = BACKGROUND_TYPE_GRADIENT;
    
    // Setup Camera
    Reset();
    
    // Load Model
    FMesh Mesh;
    if (Type == EModelSceneType::Default)
    {
        Mesh.LoadFromFile(RESOURCE_PATH"/models/queen.obj");
        m_Settings.CameraSpeed = 1.5f;
    }
    else if (Type == EModelSceneType::Sponza)
    {
        Mesh.LoadFromFile(RESOURCE_PATH"/models/sponza/sponza.obj");
        m_Settings.CameraSpeed = 150.0f;
    }

    // Copy data to the scene
    m_Vertices   = Mesh.Vertices;
    m_VerticesEx = Mesh.VerticesEx;
    m_Indicies   = Mesh.Indicies;
    m_Materials  = Mesh.Materials;
    
    // Build BVH
    m_AccelerationStructure.Build(Mesh, 32);
    
    std::cout << "Depth: " << m_AccelerationStructure.Stats.Depth << "\n";
    std::cout << "MaxTrianglesInLeafNode: " << m_AccelerationStructure.Stats.MaxTrianglesInLeafNode << "\n";
    std::cout << "Num BoundingBoxes: " << m_AccelerationStructure.m_BoundingBoxes.size() << "\n";
    
    // Mesh Data
    m_Meshes.push_back(
    {
        0, // BoundingBoxIndex
        4, // MaterialIndex
    });
    
    // Cache Device
    FDevice* pDevice = FApplication::Get().GetDevice();
    
    // BVH Buffers
    FBufferParams BufferParams;
    BufferParams.Size             = sizeof(FShaderBoundingBox) * m_AccelerationStructure.m_BoundingBoxes.size();
    BufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;
    BufferParams.Usage            = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    assert(m_AccelerationStructure.m_BoundingBoxes.size() < MAX_BVH_NODES);
    m_pBoundingBoxBuffer = FBuffer::CreateWithData(pDevice, BufferParams, nullptr, m_AccelerationStructure.m_BoundingBoxes.data());
    assert(m_pBoundingBoxBuffer != nullptr);
    m_pBoundingBoxBuffer->SetDebugName("CPU Bounding Box Buffer");
    
    BufferParams.Size = sizeof(FShaderTriangle) * m_AccelerationStructure.m_Triangles.size();
    m_pTriangleBuffer = FBuffer::CreateWithData(pDevice, BufferParams, nullptr, m_AccelerationStructure.m_Triangles.data());
    assert(m_pTriangleBuffer != nullptr);
    m_pTriangleBuffer->SetDebugName("CPU Triangle Buffer");
    
    BufferParams.Size = sizeof(FVertexPosOnly) * m_Vertices.size();
    m_pVertexBuffer = FBuffer::CreateWithData(pDevice, BufferParams, nullptr, m_Vertices.data());
    assert(m_pVertexBuffer != nullptr);
    m_pVertexBuffer->SetDebugName("CPU Vertex Buffer");
    
    BufferParams.Size = sizeof(FVertexEx) * m_VerticesEx.size();
    m_pVertexExBuffer = FBuffer::CreateWithData(pDevice, BufferParams, nullptr, m_VerticesEx.data());
    assert(m_pVertexExBuffer != nullptr);
    m_pVertexExBuffer->SetDebugName("CPU VertexEx Buffer");
    
    BufferParams.Size             = sizeof(FVertexPosOnly) * Mesh.Vertices.size();
    BufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    BufferParams.Usage            = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    m_pMeshVertexBuffer = FBuffer::CreateWithData(pDevice, BufferParams, nullptr, Mesh.Vertices.data());
    assert(m_pMeshVertexBuffer != nullptr);
    m_pMeshVertexBuffer->SetDebugName("CPU Debug Vertex Buffer");
    
    BufferParams.Size  = sizeof(uint32_t) * Mesh.Indicies.size();
    BufferParams.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    m_pMeshIndexBuffer = FBuffer::CreateWithData(pDevice, BufferParams, nullptr, Mesh.Indicies.data());
    assert(m_pMeshIndexBuffer != nullptr);
    m_pMeshIndexBuffer->SetDebugName("CPU Debug Index Buffer");
    
    // Create a matrix for each AABB
    std::vector<glm::mat4> AABBMatrices;
    AABBMatrices.reserve(m_AccelerationStructure.m_BoundingBoxes.size());
    
    for (size_t i = 0; i < m_AccelerationStructure.m_BoundingBoxes.size(); i++)
    {
        const FShaderBoundingBox& BoundingBox = m_AccelerationStructure.m_BoundingBoxes[i];
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
    
    BufferParams.Size             = sizeof(glm::mat4) * AABBMatrices.size();
    BufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;
    BufferParams.Usage            = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    m_pAABBInstanceBuffer = FBuffer::CreateWithData(pDevice, BufferParams, nullptr, AABBMatrices.data());
    assert(m_pAABBInstanceBuffer != nullptr);
    m_pAABBInstanceBuffer->SetDebugName("CPU Debug AABB Instance Buffer");
    
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
    
    // Create materials for the materials
    for (const auto& Material : m_Materials)
    {
        FShaderMaterial& ShaderMaterial = m_GpuMaterials.emplace_back();
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
        if (Material.AlbedoTex)
        {
            ShaderMaterial.AlbedoTexIndex = pDevice->GetBindlessManager().AddImageView(Material.AlbedoTex->GetTextureView()->GetImageView(), m_pMaterialSampler->GetSampler());
        }
        else
        {
            ShaderMaterial.AlbedoTexIndex = FBindlessManager::InvalidBindlessID;
        }
    }
    
    // Quads
#if 1
    if (Type == EModelSceneType::Default)
    {
        // Floor Quad
        m_Quads.push_back({ glm::vec4(-1.0f, 0.0f, -1.0f, 0.0f), glm::vec4(0.0f, 0.0f, 2.0f, 0.0f), glm::vec4(2.0f, 0.0f, 0.0f, 0.0f), 0 });
        // Front Quad
        m_Quads.push_back({ glm::vec4(-1.0f, 2.0f, -1.0f, 0.0f), glm::vec4(0.0f, -2.0f, 0.0f, 0.0f), glm::vec4(2.0f, 0.0f, 0.0f, 0.0f), 0 });
        // Roof Quad
        m_Quads.push_back({ glm::vec4(-1.0f, 2.0f, 1.0f, 0.0f), glm::vec4(0.0f, 0.0f, -2.0f, 0.0f), glm::vec4(2.0f, 0.0f, 0.0f, 0.0f), 0 });
        // Right Quad
        m_Quads.push_back({ glm::vec4(-1.0f, 0.0f, -1.0f, 0.0f), glm::vec4(0.0f, 2.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 2.0f, 0.0f), 1 });
        // Left Quad
        m_Quads.push_back({ glm::vec4(1.0f, 2.0f, -1.0f, 0.0f), glm::vec4(0.0f, -2.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 2.0f, 0.0f), 2 });
        // Light Quad
        m_Quads.push_back({ glm::vec4(0.5f, 1.999f, 0.2f, 0.0f), glm::vec4(0.0f, 0.0f, -0.4f, 0.0f), glm::vec4(0.4f, 0.0f, 0.0f, 0.0f), 3 });
    }
#endif
    
    // Materials
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
        FBindlessManager::InvalidBindlessID,
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
        FBindlessManager::InvalidBindlessID,
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
        FBindlessManager::InvalidBindlessID,
        // padding
        0, 0
    });
    
    // Light
    m_GpuMaterials.push_back(
    {
        glm::vec4( 0.0f,  0.0f,  0.0f, 1.0f),
        glm::vec4(40.0f, 36.0f, 28.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        FBindlessManager::InvalidBindlessID,
        // padding
        0, 0
    });
    
    m_GpuMaterials.push_back(
    {
        glm::vec4(0.9f, 0.9f, 0.9f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.9f, 0.9f, 0.9f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        0.1f,
        0.9f,
        1.0f,
        0.0f,
        0.0f,
        FBindlessManager::InvalidBindlessID,
        // padding
        0, 0
    });
}

void FModelScene::Reset()
{
    m_Camera.Reset();
    
    glm::vec3 Translation(0.0f, 0.5f, 1.75f);
    m_Camera.Move(Translation);

    glm::vec3 Rotation(0.0f, glm::pi<float>(), 0.0f);
    m_Camera.Rotate(Rotation);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Spheres

void FSphereScene::Initialize()
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
            FBindlessManager::InvalidBindlessID,
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
            FBindlessManager::InvalidBindlessID,
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
            FBindlessManager::InvalidBindlessID,
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
            FBindlessManager::InvalidBindlessID,
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
            FBindlessManager::InvalidBindlessID,
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
            FBindlessManager::InvalidBindlessID,
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
            0.0f,
            0.0f,
            0.0f,
            FBindlessManager::InvalidBindlessID,
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
                    FBindlessManager::InvalidBindlessID,
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
                    FBindlessManager::InvalidBindlessID,
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
                    FBindlessManager::InvalidBindlessID,
                    // padding
                    0, 0
                });
            }
        }
    }
}

void FSphereScene::Reset()
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

void FCornellBoxScene::Initialize()
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
    // Back Quad - NOTE: Disabled to let some light into the box for now
    // m_Quads.push_back({ glm::vec4(-2.0f, 0.0f, 2.0f, 0.0f), glm::vec4(0.0f, 4.0f, 0.0f, 0.0f), glm::vec4(4.0f, 0.0f, 0.0f, 0.0f), 4 });
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
        FBindlessManager::InvalidBindlessID,
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
        FBindlessManager::InvalidBindlessID,
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
        FBindlessManager::InvalidBindlessID,
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
        FBindlessManager::InvalidBindlessID,
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
        FBindlessManager::InvalidBindlessID,
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
        FBindlessManager::InvalidBindlessID,
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
        FBindlessManager::InvalidBindlessID,
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
        FBindlessManager::InvalidBindlessID,
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
        FBindlessManager::InvalidBindlessID,
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
        FBindlessManager::InvalidBindlessID,
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
        FBindlessManager::InvalidBindlessID,
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
        FBindlessManager::InvalidBindlessID,
        // padding
        0, 0
    });
}

void FCornellBoxScene::Reset()
{
    m_Camera.Reset();

    glm::vec3 Translation(0.0f, 3.5f, 5.0f);
    m_Camera.Move(Translation);

    glm::vec3 Rotation(glm::pi<float>() / 8.0f, glm::pi<float>(), 0.0f);
    m_Camera.Rotate(Rotation);
}
