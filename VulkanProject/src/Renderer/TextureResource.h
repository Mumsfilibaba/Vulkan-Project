#pragma once
#include "Vulkan/Texture.h"
#include "Vulkan/TextureView.h"
#include "Vulkan/PipelineState.h"
#include "Vulkan/PipelineLayout.h"
#include "Vulkan/Sampler.h"

class CTextureResource
{
public:
    static bool InitLoader(CDevice* pDevice);
    static void ReleaseLoader();

    static CTextureResource* LoadFromFile(CDevice* pDevice, const char* Filepath);
    static CTextureResource* LoadCubeMapFromPanoramaFile(CDevice* pDevice, const char* Filepath);
    
    CTextureResource(CDevice* pDevice);
    ~CTextureResource();

    CTexture* GetTexture() const
    {
        return m_pTexture;
    }

    CTextureView* GetTextureView() const
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
    CDevice*      m_pDevice;
    CTexture*     m_pTexture;
    CTextureView* m_pTextureView;
    uint32_t      m_Width;
    uint32_t      m_Height;

    static CSampler*             s_pCubeMapGenSampler;
    static CDescriptorSetLayout* s_pCubeMapGenDescriptorSetLayout;
    static CPipelineLayout*      s_pCubeMapGenPipelineLayout;
    static CComputePipeline*     s_pCubeMapGenPipelineState;
};
