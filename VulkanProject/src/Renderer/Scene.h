#pragma once
#include "Core.h"

#define MAX_QUAD (128)
#define MAX_SPHERES (32)
#define MAX_PLANES (8)
#define MAX_MATERIALS (32)

#define MATERIAL_LAMBERTIAN (1)
#define MATERIAL_METAL (2)
#define MATERIAL_EMISSIVE (3)
#define MATERIAL_DIELECTRIC (4)

struct Sphere
{
    glm::vec3 Position;
    float     Radius;
    uint32_t  MaterialIndex;
    uint32_t  Padding0;
    uint32_t  Padding1;
    uint32_t  Padding2;
};

struct Plane
{
    glm::vec3 Normal;
    float     Distance;
    uint32_t  MaterialIndex;
    uint32_t  Padding0;
    uint32_t  Padding1;
    uint32_t  Padding2;
};

struct Quad
{
    glm::vec4 Position;
    glm::vec4 Edge0;
    glm::vec4 Edge1;
    uint32_t  MaterialIndex;
    uint32_t  Padding0;
    uint32_t  Padding1;
    uint32_t  Padding2;
};

struct Material
{
    glm::vec4 Albedo;
    glm::vec4 Emissive;
    uint32_t  Type;
    float     Roughness;
    float     RefractionIndex;
    uint32_t  Padding1;
};

struct SceneSettings
{
    uint32_t bUseGlobalLight;
};

struct Scene
{
    Scene();
    ~Scene() = default;

    virtual void Initialize() {}

    std::vector<Quad>     m_Quads;
    std::vector<Sphere>   m_Spheres;
    std::vector<Plane>    m_Planes;
    std::vector<Material> m_Materials;
    SceneSettings         m_Settings;
};

struct SphereScene : public Scene
{
    virtual void Initialize() override;
};

struct CornellBoxScene : public Scene
{
    virtual void Initialize() override;
};