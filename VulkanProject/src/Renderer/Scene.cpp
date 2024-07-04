#include "Scene.h"
#include "Model.h"
#include "Application.h"
#include "Vulkan/Buffer.h"

FScene::FScene()
    : m_Quads()
    , m_Spheres()
    , m_Materials()
    , m_Settings()
    , m_pVertexBuffer(nullptr)
    , m_pTriangleBuffer(nullptr)
    , m_pBoundingBoxBuffer(nullptr)
    , m_pMeshVertexBuffer(nullptr)
    , m_pMeshIndexBuffer(nullptr)
{
    m_Settings.ViewMode    = EViewMode::Render;
    m_Settings.Exposure    = 0.5f;
    m_Settings.NumBounces  = 8;
    m_Settings.FieldOfView = 90.0f;
    m_Settings.CameraSpeed = 1.5f;
    
    m_Quads.reserve(MAX_QUADS);
    m_Spheres.reserve(MAX_SPHERES);
    m_Materials.reserve(MAX_MATERIALS);
    m_Vertices.reserve(MAX_VERTICES);
    m_Meshes.reserve(MAX_TRIANGLEMESHES);
}

FScene::~FScene()
{
    // SAFE_DELETE(m_pVertexBuffer);
    // SAFE_DELETE(m_pTriangleBuffer);
    // SAFE_DELETE(m_BoundingBoxBuffer);
    // SAFE_DELETE(m_pMeshVertexBuffer);
    // SAFE_DELETE(m_pMeshIndexBuffer);
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
        Mesh.LoadFromFile("res/models/queen.obj");
    }
    else if (Type == EModelSceneType::Sponza)
    {
        Mesh.LoadFromFile("res/models/sponza/sponza.obj");
    }

    // Copy vertices
    m_Vertices = Mesh.m_Positions;
    
    // Build BVH
    m_AccelerationStructure.Build(Mesh, 4);
    
    std::cout << "Depth: " << m_AccelerationStructure.Stats.Depth << "\n";
    std::cout << "MaxTrianglesInLeafNode: " << m_AccelerationStructure.Stats.MaxTrianglesInLeafNode << "\n";
    
    // Mesh Data
    m_Meshes.push_back(
    {
        0, // BoundingBoxIndex
        4, // MaterialIndex
    });
    
    // BVH Buffers
    FBufferParams BufferParams;
    BufferParams.Size             = sizeof(FShaderBoundingBox) * m_AccelerationStructure.m_BoundingBoxes.size();
    BufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;
    BufferParams.Usage            = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    assert(m_AccelerationStructure.m_BoundingBoxes.size() < MAX_BVH_NODES);
    m_pBoundingBoxBuffer = FBuffer::CreateWithData(FApplication::Get().GetDevice(), BufferParams, nullptr, m_AccelerationStructure.m_BoundingBoxes.data());
    assert(m_pBoundingBoxBuffer != nullptr);
    
    BufferParams.Size = sizeof(FShaderTriangle) * m_AccelerationStructure.m_Triangles.size();
    m_pTriangleBuffer = FBuffer::CreateWithData(FApplication::Get().GetDevice(), BufferParams, nullptr, m_AccelerationStructure.m_Triangles.data());
    assert(m_pTriangleBuffer != nullptr);
    
    BufferParams.Size = sizeof(FVertexPosOnly) * m_Vertices.size();
    m_pVertexBuffer = FBuffer::CreateWithData(FApplication::Get().GetDevice(), BufferParams, nullptr, m_Vertices.data());
    assert(m_pVertexBuffer != nullptr);
    
    BufferParams.Size             = sizeof(FVertexPosOnly) * Mesh.m_Positions.size();
    BufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;
    BufferParams.Usage            = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    m_pMeshVertexBuffer = FBuffer::CreateWithData(FApplication::Get().GetDevice(), BufferParams, nullptr, Mesh.m_Positions.data());
    assert(m_pMeshVertexBuffer != nullptr);
    
    BufferParams.Size  = sizeof(uint32_t) * Mesh.m_Indicies.size();
    BufferParams.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    m_pMeshIndexBuffer = FBuffer::CreateWithData(FApplication::Get().GetDevice(), BufferParams, nullptr, Mesh.m_Indicies.data());
    assert(m_pMeshIndexBuffer != nullptr);
    
    // Quads
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
    
    // Materials
    m_Materials.push_back(
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
        // padding
        0, 0, 0
    });
    
    m_Materials.push_back(
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
        // padding
        0, 0, 0
    });
    
    m_Materials.push_back(
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
        // padding
        0, 0, 0
    });
    
    // Light
    m_Materials.push_back(
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
        // padding
        0, 0, 0
    });
    
    m_Materials.push_back(
    {
        glm::vec4(0.9f, 0.9f, 0.9f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.9f, 0.9f, 0.9f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        0.1f,
        0.2f,
        1.0f,
        0.0f,
        0.0f,
        // padding
        0, 0, 0
    });
}

void FModelScene::Reset()
{
    m_Camera.Reset();
    
    glm::vec3 translation(0.0f, 0.5f, 1.75f);
    m_Camera.Move(translation);

    glm::vec3 rotation(0.0f, glm::pi<float>(), 0.0f);
    m_Camera.Rotate(rotation);
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
        m_Materials.push_back(
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
            // padding
            0, 0, 0
        });
        
        // Middle Ball (Pink Ball)
        m_Materials.push_back(
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
            // padding
            0, 0, 0
        });
        
        // Left Ball (White Ball)
        m_Materials.push_back(
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
            // padding
            0, 0, 0
        });
        
        // Large Ball (Green Ball)
        m_Materials.push_back(
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
            // padding
            0, 0, 0
        });
    }
    else
    {
        constexpr int32_t numSpheres        = 7;
        constexpr float sphereRadius        = 2.8f;
        constexpr float sphereDiameter      = sphereRadius * 2.0f;
        constexpr float sphereOffset        = 0.2f;
        constexpr float sphereHalfFootPrint = sphereRadius + sphereOffset;
        constexpr float sphereFootPrint     = sphereHalfFootPrint * 2.0f;
        constexpr float width               = sphereFootPrint * numSpheres;
        constexpr float halfWidth           = width / 2.0f;
        
        // Settings
        m_Settings.BackgroundType = BACKGROUND_TYPE_SKYBOX;
        
        // Roof Quad
        constexpr float roofPos        = 23.0f;
        constexpr float roofWidth      = 15.0f;
        constexpr float roofHalfWidth  = roofWidth / 2.0f;
        m_Quads.push_back({ glm::vec4(-roofHalfWidth, roofPos, roofHalfWidth, 0.0f), glm::vec4(0.0f, 0.0f, -roofWidth, 0.0f), glm::vec4(roofWidth, 0.0f, 0.0f, 0.0f), 0 });

        // Light Quad
        constexpr float lightPos       = roofPos - 0.1f;
        constexpr float lightWidth     = 10.0f;
        constexpr float lightHalfWidth = lightWidth / 2.0;
        m_Quads.push_back({ glm::vec4(-lightHalfWidth, lightPos, lightHalfWidth, 0.0f), glm::vec4(0.0f, 0.0f, -lightWidth, 0.0f), glm::vec4(lightWidth, 0.0f, 0.0f, 0.0f), 2 });
        
        // Floor Quad
        constexpr float floorWidth     = width + (sphereFootPrint * 2.0f);
        constexpr float floorHalfWidth = floorWidth / 2.0f;
        constexpr float floorDepth     = sphereFootPrint + sphereRadius;
        constexpr float floorHalfDepth = floorDepth / 2.0f;
        m_Quads.push_back({ glm::vec4(-floorHalfWidth, -2.0f, -floorHalfDepth, 0.0f), glm::vec4(0.0f, 0.0f, floorDepth, 0.0f), glm::vec4(floorWidth, 0.0f, 0.0f, 0.0f), 0 });
        
        // Wall Quad
    #if 1
        constexpr uint32_t numQuads = 100;
        constexpr float totalWidth = floorWidth;
        constexpr float quadWidth  = totalWidth / numQuads;
        for (uint32_t i = 0; i < numQuads; i++)
        {
            const uint32_t materialIndex = i % 2;
            m_Quads.push_back(
            {
                glm::vec4(-floorHalfWidth + (quadWidth * static_cast<float>(i)), 9.0f, -floorDepth, 0.0f),
                glm::vec4(0.0f, -9.0, 0.0f, 0.0f),
                glm::vec4(quadWidth, 0.0f, 0.0f, 0.0f),
                materialIndex
            });
        }
        
    #endif
        
        // Roof-, Floor- and Wall- Material
        m_Materials.push_back(
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
            // padding
            0, 0, 0
        });
        
        // Wall- Material
        m_Materials.push_back(
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
            // padding
            0, 0, 0
        });
        
        // Light Material
        m_Materials.push_back(
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
            // padding
            0, 0, 0
        });
        
        // Spheres
        const uint32_t startMaterialIndex = static_cast<uint32_t>(m_Materials.size());
        for (int32_t i = 0; i < numSpheres; i++)
        {
            const float SphereStartPos = -(sphereHalfFootPrint - halfWidth);
            m_Spheres.push_back(
            {
                glm::vec3(SphereStartPos - (static_cast<float>(i) * sphereFootPrint), sphereRadius + sphereOffset, 0.0f),
                sphereRadius,
                startMaterialIndex + static_cast<uint32_t>(i)
            });
        }
        
        if (Type == ESphereSceneType::PolishedGlass)
        {
            for (int32_t i = 0; i < numSpheres; i++)
            {
                const float incidenceOfRefraction = 1.0f + 0.5f * float(i) / float(numSpheres - 1);
                m_Materials.push_back(
                {
                    glm::vec4(0.9f, 0.25f, 0.25f, 1.0f),
                    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                    glm::vec4(0.8f, 0.8f, 0.8f, 1.0f),
                    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                    0.02f,
                    0.0f,
                    incidenceOfRefraction,
                    1.0f,
                    0.0f,
                    // padding
                    0, 0, 0
                });
            }
        }
        else if (Type == ESphereSceneType::ColoredRoughGlass)
        {
            for (int32_t i = 0; i < numSpheres; i++)
            {
                const float roughness = float(i) / float(numSpheres - 1) * 0.5f;
                m_Materials.push_back(
                {
                    glm::vec4(0.9f, 0.25f, 0.25f, 1.0f),
                    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                    glm::vec4(0.8f, 0.8f, 0.8f, 1.0f),
                    glm::vec4(0.0f, 0.5f, 1.0f, 1.0f),
                    0.02f,
                    roughness,
                    1.1f,
                    1.0f,
                    roughness,
                    // padding
                    0, 0, 0
                });
            }
        }
        else if (Type == ESphereSceneType::RoughGlass)
        {
            for (int32_t i = 0; i < numSpheres; i++)
            {
                const float roughness = float(i) / float(numSpheres - 1) * 0.5f;
                m_Materials.push_back(
                {
                    glm::vec4(0.9f, 0.25f, 0.25f, 1.0f),
                    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                    glm::vec4(0.8f, 0.8f, 0.8f, 1.0f),
                    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                    0.02f,
                    roughness,
                    1.1f,
                    1.0f,
                    roughness,
                    // padding
                    0, 0, 0
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
        glm::vec3 translation(0.0f, 1.0f, 0.75f);
        m_Camera.Move(translation);
        
        glm::vec3 rotation(glm::pi<float>() / 4.0f, 0.0f, 0.0f);
        m_Camera.Rotate(rotation);
    }
    else
    {
        m_Settings.CameraSpeed = 10.0f;
        
        glm::vec3 translation(0.0f, 8.0f, 24.0f);
        m_Camera.Move(translation);

        glm::vec3 rotation(0.0f, glm::pi<float>(), 0.0f);
        m_Camera.Rotate(rotation);
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
    m_Materials.push_back(
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
        // padding
        0, 0, 0
    });
    
    m_Materials.push_back(
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
        // padding
        0, 0, 0
    });
    
    m_Materials.push_back(
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
        // padding
        0, 0, 0
    });
    
    // Light
    m_Materials.push_back(
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
        // padding
        0, 0, 0
    });
    
    // Green Materials
    m_Materials.push_back(
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
        // padding
        0, 0, 0
    });
    
    m_Materials.push_back(
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
        // padding
        0, 0, 0
    });
    
    m_Materials.push_back(
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
        // padding
        0, 0, 0
    });
    
    m_Materials.push_back(
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
        // padding
        0, 0, 0
    });
    
    m_Materials.push_back(
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
        // padding
        0, 0, 0
    });
    
    // Ball Materials
    m_Materials.push_back(
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
        // padding
        0, 0, 0
    });
    
    m_Materials.push_back(
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
        // padding
        0, 0, 0
    });
    
    m_Materials.push_back(
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
        // padding
        0, 0, 0
    });
}

void FCornellBoxScene::Reset()
{
    m_Camera.Reset();

    glm::vec3 translation(0.0f, 3.5f, 5.0f);
    m_Camera.Move(translation);

    glm::vec3 rotation(glm::pi<float>() / 8.0f, glm::pi<float>(), 0.0f);
    m_Camera.Rotate(rotation);
}
