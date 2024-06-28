#pragma once
#include "Camera.h"
#include "Model.h"
#include "GPUPrimitives.h"

#define MAX_QUAD 128
#define MAX_SPHERES 32
#define MAX_PLANES 32
#define MAX_MATERIALS 32
#define MAX_TRIANGLES 1024

#define BACKGROUND_TYPE_NONE 0
#define BACKGROUND_TYPE_GRADIENT 1
#define BACKGROUND_TYPE_SKYBOX 2

struct FSceneSettings
{
    uint32_t BackgroundType;
    float    Exposure;
    uint32_t NumBounces;
    float    FieldOfView;
    float    CameraSpeed;
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

enum class ESphereSceneType
{
    Default = 1,
    ColoredRoughGlass = 2,
    PolishedGlass = 3,
    RoughGlass = 4, 
};

struct FSphereScene : public FScene
{
    FSphereScene(ESphereSceneType InType)
        : Type(InType)
    {
    }
    
    virtual void Initialize() override;

    virtual void Reset() override;
    
    const ESphereSceneType Type;
};

struct FCornellBoxScene : public FScene
{
    virtual void Initialize() override;

    virtual void Reset() override;
};
