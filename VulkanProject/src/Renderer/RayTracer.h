#pragma once
#include "Core.h"
#include "IRenderer.h"
#include "Camera.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Primitives

#define MAX_QUAD    (128)
#define MAX_SPHERES (32)
#define MAX_PLANES  (8)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Materials

#define MAX_MATERIALS (32)

class Buffer;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Buffer Structs

struct RandomBuffer
{
    uint32_t FrameIndex  = 0;
    uint32_t SampleIndex = 0;
    uint32_t NumSamples  = 0;
    uint32_t Padding0    = 0;
};

struct SceneBuffer
{
    uint32_t NumQuads     = 0;
    uint32_t NumSpheres   = 0;
    uint32_t NumPlanes    = 0;
    uint32_t NumMaterials = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Primitive Structs

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Material

#define MATERIAL_LAMBERTIAN (1)
#define MATERIAL_METAL      (2)
#define MATERIAL_EMISSIVE   (3)

struct Material
{
    glm::vec4 Albedo;
    glm::vec4 Emissive;
    float     Roughness;
    uint32_t  Type;
    uint32_t  Padding0;
    uint32_t  Padding1;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// RayTracer

class RayTracer : public IRenderer
{
public:
    RayTracer();
    ~RayTracer() = default;
    
    virtual void Init(Device* pDevice, Swapchain* pSwapchain) override;
    
    virtual void Release() override;
    
    virtual void Tick(float deltaTime) override;
    
    virtual void OnRenderUI() override;
    
    virtual void OnWindowResize(uint32_t width, uint32_t height) override;
    
private:
    void CreateOrResizeSceneTexture(uint32_t width, uint32_t height);

    void CreateDescriptorSet();
    void ReleaseDescriptorSet();

    void ReloadShader();

    Device*                       m_pDevice;
    Swapchain*                    m_pSwapchain;
    std::atomic<ComputePipeline*> m_pPipeline;
    class PipelineLayout*         m_pPipelineLayout;
    class DescriptorSetLayout*    m_pDescriptorSetLayout;
    DeviceMemoryAllocator*        m_pDeviceAllocator;
    DescriptorPool*               m_pDescriptorPool;
    class DescriptorSet*          m_pDescriptorSet;

    std::vector<class CommandBuffer*> m_CommandBuffers;
    std::vector<class Query*>         m_TimestampQueries;
    
    Buffer* m_pCameraBuffer;
    Buffer* m_pRandomBuffer;
    Buffer* m_pSceneBuffer;
    Buffer* m_pSphereBuffer;
    Buffer* m_pPlaneBuffer;
    Buffer* m_pQuadBuffer;
    Buffer* m_pMaterialBuffer;

    std::vector<Quad>     m_Quads;
    std::vector<Sphere>   m_Spheres;
    std::vector<Plane>    m_Planes;
    std::vector<Material> m_Materials;

    class Texture*       m_pAccumulationTexture;
    class TextureView*   m_pAccumulationTextureView;
    class Texture*       m_pSceneTexture;
    class TextureView*   m_pSceneTextureView;
    class DescriptorSet* m_pSceneTextureDescriptorSet;

    Camera           m_Camera;

    uint32_t         m_NumSamples;
    std::atomic_bool m_bResetImage;

    float    m_LastCPUTime;
    float    m_LastGPUTime;

    uint32_t m_ViewportWidth;
    uint32_t m_ViewportHeight;
};
