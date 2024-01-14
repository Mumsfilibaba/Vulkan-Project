#include "Scene.h"

FScene::FScene()
    : m_Quads()
    , m_Spheres()
    , m_Planes()
    , m_Materials()
    , m_Settings()
{
    m_Quads.reserve(MAX_QUAD);
    m_Spheres.reserve(MAX_SPHERES);
    m_Planes.reserve(MAX_PLANES);
    m_Materials.reserve(MAX_MATERIALS);
}

void FSphereScene::Initialize()
{
    // Settings
    m_Settings.BackgroundType = BACKGROUND_TYPE_GRADIENT;

    // Setup Camera
    Reset();

    // Spheres
    m_Spheres.push_back({ glm::vec3(0.0f, -100.5f, 0.0f), 100.0f, 0 });
    m_Spheres.push_back({ glm::vec3(0.0f,    0.0f, 1.0f),  0.49f, 1 });

    m_Spheres.push_back({ glm::vec3(-1.0f, 0.0f, 1.0f), -0.47f, 2 });
    m_Spheres.push_back({ glm::vec3(-1.0f, 0.0f, 1.0f),  0.49f, 2 });
    m_Spheres.push_back({ glm::vec3( 1.0f, 0.0f, 1.0f),  0.49f, 3 });

    // Materials
    m_Materials.push_back(
    {
        glm::vec4(0.8f, 0.8f, 0.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        MATERIAL_LAMBERTIAN,
        1.0f,
        0.0f,
    });
    m_Materials.push_back(
    {
        glm::vec4(0.7f, 0.3f, 0.3f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        MATERIAL_LAMBERTIAN,
        1.0f,
        0.0f,
    });
    m_Materials.push_back(
    {
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        MATERIAL_DIELECTRIC,
        0.3f,
        1.5f,
    });
    m_Materials.push_back(
    {
        glm::vec4(0.8f, 0.6f, 0.2f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        MATERIAL_METAL,
        0.3f,
        0.0f,
    });
}

void FSphereScene::Reset()
{
    m_Camera.Reset();

    glm::vec3 translation(0.0f, 0.0f, 1.0f);
    m_Camera.Move(translation);

    glm::vec3 rotation(glm::pi<float>() / 4.0f, 0.0f, 0.0f);
    m_Camera.Rotate(rotation);
}


void FCornellBoxScene::Initialize()
{
    // Settings
    m_Settings.BackgroundType = BACKGROUND_TYPE_NONE;
    
    // Setup Camera
    Reset();

    // Quads
    m_Quads.push_back(
    {
        glm::vec4(-2.0f, 0.0f, -2.0f, 0.0f),
        glm::vec4( 0.0f, 0.0f,  4.0f, 0.0f),
        glm::vec4( 4.0f, 0.0f,  0.0f, 0.0f),
        0
    });
    m_Quads.push_back(
    {
        glm::vec4(-2.0f, 0.0f, -2.0f, 0.0f),
        glm::vec4( 0.0f, 4.0f,  0.0f, 0.0f),
        glm::vec4( 4.0f, 0.0f,  0.0f, 0.0f),
        0
    });
    m_Quads.push_back(
    {
        glm::vec4(-2.0f, 4.0f, -2.0f, 0.0f),
        glm::vec4( 0.0f, 0.0f,  4.0f, 0.0f),
        glm::vec4( 4.0f, 0.0f,  0.0f, 0.0f),
        0
    });
    m_Quads.push_back(
    {
        glm::vec4(-2.0f, 0.0f,  2.0f, 0.0f),
        glm::vec4( 0.0f, 4.0f,  0.0f, 0.0f),
        glm::vec4( 0.0f, 0.0f, -4.0f, 0.0f),
        1
    });
    m_Quads.push_back(
    {
        glm::vec4( 2.0f, 4.0f, -2.0f, 0.0f),
        glm::vec4( 0.0f,-4.0f,  0.0f, 0.0f),
        glm::vec4( 0.0f, 0.0f,  4.0f, 0.0f),
        2
    });
    m_Quads.push_back(
    {
        glm::vec4(-0.35f, 3.995f, -0.35f, 0.0f),
        glm::vec4(  0.0f,   0.0f,   0.7f, 0.0f),
        glm::vec4(  0.7f,   0.0f,   0.0f, 0.0f),
        3
    });

    // Spheres
    m_Spheres.push_back({ glm::vec3(-1.15f, 0.725f,   0.0f),   0.7f, 4 });
    m_Spheres.push_back({ glm::vec3(  0.0f, 0.725f, -1.15f),   0.7f, 5 });
    m_Spheres.push_back({ glm::vec3( 1.15f, 0.725f,   0.0f),   0.7f, 6 });
    m_Spheres.push_back({ glm::vec3( 1.15f, 0.725f,   0.0f), -0.65f, 6 });

    // Materials
    m_Materials.push_back(
    {
        glm::vec4(0.73f, 0.73f, 0.73f, 1.0f),
        glm::vec4( 0.0f,  0.0f,  0.0f, 1.0f),
        MATERIAL_LAMBERTIAN,
        0.0f,
        0.0f
    });
    m_Materials.push_back(
    {
        glm::vec4(0.65f, 0.05f, 0.05f, 1.0f),
        glm::vec4( 0.0f,  0.0f,  0.0f, 1.0f),
        MATERIAL_LAMBERTIAN,
        0.0f,
        0.0f
    });
    m_Materials.push_back(
    {
        glm::vec4(0.12f, 0.45f, 0.15f, 1.0f),
        glm::vec4( 0.0f,  0.0f,  0.0f, 1.0f),
        MATERIAL_LAMBERTIAN,
        0.0f,
        0.0f
    });
    m_Materials.push_back(
    {
        glm::vec4( 0.0f,  0.0f,  0.0f, 1.0f),
        glm::vec4(30.0f, 30.0f, 30.0f, 1.0f),
        MATERIAL_EMISSIVE,
        0.0f,
        0.0f
    });
    m_Materials.push_back(
    {
        glm::vec4(0.1f, 0.1f, 0.7f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        MATERIAL_LAMBERTIAN,
        0.0f,
        0.0f
    });
    m_Materials.push_back(
    {
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
        MATERIAL_METAL,
        0.0f,
        0.0f
    });
    m_Materials.push_back(
    {
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        MATERIAL_DIELECTRIC,
        0.3f,
        1.5f,
    });
}

void FCornellBoxScene::Reset()
{
    m_Camera.Reset();

    glm::vec3 translation(0.0f, 2.5f, 5.0f);
    m_Camera.Move(translation);

    glm::vec3 rotation(glm::pi<float>() / 8.0f, glm::pi<float>(), 0.0f);
    m_Camera.Rotate(rotation);
}
