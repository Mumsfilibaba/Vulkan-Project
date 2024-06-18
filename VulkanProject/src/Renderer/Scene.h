#pragma once
#include "Camera.h"
#include "Model.h"

#define MAX_QUAD 128
#define MAX_SPHERES 32
#define MAX_PLANES 8
#define MAX_MATERIALS 32
#define MAX_TRIANGLES 1024

#define MATERIAL_LAMBERTIAN 1
#define MATERIAL_METAL 2
#define MATERIAL_EMISSIVE 3
#define MATERIAL_DIELECTRIC 4

#define BACKGROUND_TYPE_NONE 0
#define BACKGROUND_TYPE_GRADIENT 1
#define BACKGROUND_TYPE_SKYBOX 2

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

struct FTriangle
{
    // 0-12
    uint32_t Index0;
    uint32_t Index1;
    uint32_t Index2;
    // Padding
    uint32_t Padding0;
};

struct FTriangleMesh
{
    // 0-32
    glm::vec4 BoxMin;
    glm::vec4 BoxMax;
    // 32-40
    uint32_t  StartTriangle;
    uint32_t  NumTriangles;
    // Padding
    uint32_t  Padding0;
    uint32_t  Padding1;
};

struct FMaterial
{
    // 0-32
    glm::vec4 Albedo;
    glm::vec4 Emissive;
    // 32-44
    uint32_t  Type;
    float     Roughness;
    float     RefractionIndex;
    // Padding
    uint32_t  Padding0;
};

struct FSceneSettings
{
    // 0-8
    uint32_t BackgroundType;
    float    Exposure;
    // Padding
    uint32_t Padding0;
    uint32_t Padding1;
};

struct FScene
{
    FScene();

    virtual void Initialize() {}

    virtual void Reset() {}

    FCamera        m_Camera;
    FSceneSettings m_Settings;

    // Materials
    std::vector<FMaterial> m_Materials;
    
    // Triangle Mesh Data
    std::vector<FVertexRT>     m_Vertices;
    std::vector<FTriangle>     m_Triangles;
    std::vector<FTriangleMesh> m_TriangleMeshes;
    
    // Other primitive data
    std::vector<FPlane>  m_Planes;
    std::vector<FSphere> m_Spheres;
    std::vector<FQuad>   m_Quads;
};

struct FModelScene : public FScene
{
    virtual void Initialize() override;

    virtual void Reset() override;
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
