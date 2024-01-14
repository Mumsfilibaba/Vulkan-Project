#pragma once
#include "Vulkan/Texture.h"
#include "Vulkan/TextureView.h"
#include "Vulkan/PipelineState.h"
#include "Vulkan/PipelineLayout.h"
#include "Vulkan/Sampler.h"

class FTextureResource
{
public:
    static bool InitLoader(FDevice* pDevice);
    static void ReleaseLoader();

    static FTextureResource* LoadFromFile(FDevice* pDevice, const char* filepath);
    static FTextureResource* LoadCubeMapFromPanoramaFile(FDevice* pDevice, const char* filepath);
    
    FTextureResource(FDevice* pDevice);
    ~FTextureResource();

    FTexture* GetTexture() const
    {
        return m_pTexture;
    }

    FTextureView* GetTextureView() const
    {
        return m_pTextureView;
    }

    uint32_t GetWidth() const
    {
        return m_Width;
    }

    uint32_t GetHeight() const
    {
        return m_Height;
    }

private:
    FDevice*      m_pDevice;
    FTexture*     m_pTexture;
    FTextureView* m_pTextureView;
    uint32_t      m_Width;
    uint32_t      m_Height;

    static FSampler*             s_pCubeMapGenSampler;
    static FDescriptorSetLayout* s_pCubeMapGenDescriptorSetLayout;
    static FPipelineLayout*      s_pCubeMapGenPipelineLayout;
    static FComputePipeline*     s_pCubeMapGenPipelineState;
};
