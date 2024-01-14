#pragma once
#include "Camera.h"

#define MAX_QUAD (128)
#define MAX_SPHERES (32)
#define MAX_PLANES (8)
#define MAX_MATERIALS (32)

#define MATERIAL_LAMBERTIAN (1)
#define MATERIAL_METAL (2)
#define MATERIAL_EMISSIVE (3)
#define MATERIAL_DIELECTRIC (4)

#define BACKGROUND_TYPE_NONE (0)
#define BACKGROUND_TYPE_GRADIENT (1)
#define BACKGROUND_TYPE_SKYBOX (2)

struct FSphere
{
    glm::vec3 Position;
    float     Radius;
    uint32_t  MaterialIndex;
    uint32_t  Padding0;
    uint32_t  Padding1;
    uint32_t  Padding2;
};

struct FPlane
{
    glm::vec3 Normal;
    float     Distance;
    uint32_t  MaterialIndex;
    uint32_t  Padding0;
    uint32_t  Padding1;
    uint32_t  Padding2;
};

struct FQuad
{
    glm::vec4 Position;
    glm::vec4 Edge0;
    glm::vec4 Edge1;
    uint32_t  MaterialIndex;
    uint32_t  Padding0;
    uint32_t  Padding1;
    uint32_t  Padding2;
};

struct FMaterial
{
    glm::vec4 Albedo;
    glm::vec4 Emissive;
    uint32_t  Type;
    float     Roughness;
    float     RefractionIndex;
    uint32_t  Padding1;
};

struct FSceneSettings
{
    uint32_t BackgroundType;
    float    Exposure;
};

struct FScene
{
    FScene();

    virtual void Initialize() {}

    virtual void Reset() {}

    FCamera                m_Camera;
    FSceneSettings         m_Settings;

    std::vector<FQuad>     m_Quads;
    std::vector<FSphere>   m_Spheres;
    std::vector<FPlane>    m_Planes;
    std::vector<FMaterial> m_Materials;
};

struct FSphereScene : public FScene
{
    virtual void Initialize() override;

    virtual void Reset() override;
};

struct FCornellBoxScene : public FScene
{
    virtual void Initialize() override;

    virtual void Reset() override;
};
