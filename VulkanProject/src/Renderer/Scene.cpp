#include "Scene.h"
#include "Model.h"

FScene::FScene()
    : m_Quads()
    , m_Spheres()
    , m_Planes()
    , m_Materials()
    , m_Settings()
{
    m_Settings.Exposure    = 0.5f;
    m_Settings.NumBounces  = 4;
    m_Settings.FieldOfView = 90.0f;

    m_Quads.reserve(MAX_QUAD);
    m_Spheres.reserve(MAX_SPHERES);
    m_Planes.reserve(MAX_PLANES);
    m_Materials.reserve(MAX_MATERIALS);
    m_Triangles.reserve(MAX_TRIANGLES);
    m_Vertices.reserve(MAX_TRIANGLES * 3);
    m_TriangleMeshes.reserve(2);
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
    Mesh.LoadFromFile("res/models/queen.obj");

    // Copy vertices
    m_Vertices = Mesh.m_Positions;
    
    // Convert indices to triangle
    for (uint32_t i = 0; i < Mesh.m_Indicies.size(); i += 3)
    {
        m_Triangles.push_back(FTriangle
        {
            Mesh.m_Indicies[i + 0],
            Mesh.m_Indicies[i + 1],
            Mesh.m_Indicies[i + 2],
            // Padding
            0
        });
    }
    
    // Mesh Data
    m_TriangleMeshes.push_back(
    {
        glm::vec4(Mesh.BoundingBoxMin, 0.0f),
        glm::vec4(Mesh.BoundingBoxMax, 0.0f),
        0,
        static_cast<uint32_t>(m_Triangles.size()),
        // Padding
        0, 0,
    });
    
    // Quads
    
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
    m_Quads.push_back({ glm::vec4(0.15f, 1.995f, 0.375f, 0.0f), glm::vec4(0.0f, 0.0f, -0.75f, 0.0f), glm::vec4(0.75f, 0.0f, 0.0f, 0.0f), 3 });
    
    // Materials
    m_Materials.push_back(
    {
        glm::vec4(0.7f, 0.7f, 0.7f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        1.0f,
        0.0f,
        0.0f,
        0
    });
    m_Materials.push_back(
    {
        glm::vec4(0.7f, 0.1f, 0.1f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        1.0f,
        0.0f,
        0.0f,
        0
    });
    m_Materials.push_back(
    {
        glm::vec4(0.1f, 0.7f, 0.1f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        1.0f,
        0.0f,
        0.0f,
        0
    });
    m_Materials.push_back(
    {
        glm::vec4( 0.0f,  0.0f,  0.0f, 1.0f),
        glm::vec4(20.0f, 18.0f, 14.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        0.0f,
        0.0f,
        0.0f,
        0
    });
    m_Materials.push_back(
    {
        glm::vec4(0.1f, 0.1f, 0.7f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        1.0f,
        0.0f,
        0.0f,
        0
    });
}

void FModelScene::Reset()
{
    m_Camera.Reset();
    
    glm::vec3 translation(0.0f, 1.25f, 3.0f);
    m_Camera.Move(translation);

    glm::vec3 rotation(glm::pi<float>() / 16.0f, glm::pi<float>(), 0.0f);
    m_Camera.Rotate(rotation);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Spheres

void FSphereScene::Initialize()
{
    // Settings
    m_Settings.BackgroundType = BACKGROUND_TYPE_GRADIENT;

    // Setup Camera
    Reset();

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
        1.0f,
        0.5f,
        0.0f,
        0
    });
    
    // Middle Ball (Pink Ball)
    m_Materials.push_back(
    {
        glm::vec4(0.7f, 0.3f, 0.3f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.7f, 0.3f, 0.3f, 1.0f),
        1.0f,
        0.1f,
        0.0f,
        0
    });
    
    // Left Ball (White Ball)
    m_Materials.push_back(
    {
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        0.0f,
        0.8f,
        0.0f,
        0
    });
    
    // Large Ball (Green Ball)
    m_Materials.push_back(
    {
        glm::vec4(0.7f, 0.9f, 0.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        0.0f,
        1.0f,
        0.0f,
        0
    });
}

void FSphereScene::Reset()
{
    m_Camera.Reset();

    glm::vec3 translation(0.0f, 1.0f, 0.75f);
    m_Camera.Move(translation);

    glm::vec3 rotation(glm::pi<float>() / 4.0f, 0.0f, 0.0f);
    m_Camera.Rotate(rotation);
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
        0.0f,
        0.9f,
        0.0f,
        0
    });
    
    m_Materials.push_back(
    {
        glm::vec4(0.7f, 0.1f, 0.1f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        0.0f,
        0.9f,
        0.0f,
        0
    });
    
    m_Materials.push_back(
    {
        glm::vec4(0.1f, 0.7f, 0.1f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        0.0f,
        0.9f,
        0.0f,
        0
    });
    
    // Light
    m_Materials.push_back(
    {
        glm::vec4( 0.0f,  0.0f,  0.0f, 1.0f),
        glm::vec4(20.0f, 18.0f, 14.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        0.0f,
        0.0f,
        0.0f,
        0
    });
    
    // Green Materials
    m_Materials.push_back(
    {
        glm::vec4(0.3f, 0.9f, 0.3f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.3f, 0.9f, 0.3f, 1.0f),
        1.0f,
        0.0f,
        0.0f,
        0
    });
    
    m_Materials.push_back(
    {
        glm::vec4(0.3f, 0.9f, 0.3f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.3f, 0.9f, 0.3f, 1.0f),
        1.0f,
        0.25f,
        0.0f,
        0
    });
    
    m_Materials.push_back(
    {
        glm::vec4(0.3f, 0.9f, 0.3f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.3f, 0.9f, 0.3f, 1.0f),
        1.0f,
        0.5f,
        0.0f,
        0
    });
    
    m_Materials.push_back(
    {
        glm::vec4(0.3f, 0.9f, 0.3f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.3f, 0.9f, 0.3f, 1.0f),
        1.0f,
        0.75f,
        0.0f,
        0
    });
    
    m_Materials.push_back(
    {
        glm::vec4(0.3f, 0.9f, 0.3f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.3f, 0.9f, 0.3f, 1.0f),
        1.0f,
        1.0f,
        0.0f,
        0
    });
    
    // Ball Materials
    m_Materials.push_back(
    {
        glm::vec4(0.9f, 0.9f, 0.75f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.9f, 0.9f, 0.9f, 1.0f),
        0.1f,
        0.2f,
        0.0f,
        0
    });
    
    m_Materials.push_back(
    {
        glm::vec4(0.9f, 0.75f, 0.9f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.9f, 0.9f, 0.9f, 1.0f),
        0.5f,
        0.2f,
        0.0f,
        0
    });
    
    m_Materials.push_back(
    {
        glm::vec4(0.75f, 0.9f, 0.9f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        glm::vec4(0.9f, 0.9f, 0.9f, 1.0f),
        1.0f,
        0.2f,
        0.0f,
        0
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
