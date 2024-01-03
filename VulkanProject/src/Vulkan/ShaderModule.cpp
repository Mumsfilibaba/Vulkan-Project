#include "ShaderModule.h"
#include "VulkanContext.h"
#include <fstream>
#include <iostream>

ShaderModule* ShaderModule::Create(VulkanContext* pContext, const uint32_t* pByteCode, uint32_t byteCodeLength, const char* pEntryPoint)
{
    ShaderModule* pShader = new ShaderModule(pContext->GetDevice());
    assert(pEntryPoint != nullptr);
    assert(pByteCode != nullptr);
    assert(byteCodeLength != 0);

    VkShaderModuleCreateInfo createInfo;
    ZERO_STRUCT(&createInfo);
    
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = byteCodeLength;
    createInfo.pCode    = pByteCode;

    VkResult result = vkCreateShaderModule(pContext->GetDevice(), &createInfo, nullptr, &pShader->m_Module);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkCreateShaderModule failed" << std::endl;
        return nullptr;
    }
    else
    {
        const size_t len = strlen(pEntryPoint);
        pShader->m_pEntryPoint = new char[len + 1];
        strcpy(pShader->m_pEntryPoint, pEntryPoint);

        std::cout << "Created ShaderModule" << std::endl;
        return pShader;
    }
}

ShaderModule* ShaderModule::CreateFromFile(VulkanContext* pContext, const char* pEntryPoint, const char* pFilePath)
{
    if (!pFilePath)
    {
        std::cout << "Not a valid filename" << std::endl;
        return nullptr;
    }

    std::string filepath = std::string(pFilePath);
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (file.is_open()) 
    {
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
                
        ShaderModule* newShader = ShaderModule::Create(pContext, reinterpret_cast<const uint32_t*>(buffer.data()), buffer.size(), pEntryPoint);
        if (!newShader)
        {
            return nullptr;
        }
        
        std::cout << "Loaded Shader '" << filepath << "'" << std::endl;
        return newShader;
    }
    else
    {
        std::cout << "Failed to open file '" << filepath << "'" << std::endl;
        return nullptr;
    }
}

ShaderModule::ShaderModule(VkDevice device)
    : m_Device(device)
    , m_Module(VK_NULL_HANDLE)
    , m_pEntryPoint(nullptr)
{
}

ShaderModule::~ShaderModule()
{
    if (m_Module)
    {
        vkDestroyShaderModule(m_Device, m_Module, nullptr);
        m_Module = VK_NULL_HANDLE;
    }

    if (m_pEntryPoint)
    {
        delete m_pEntryPoint;
        m_pEntryPoint = nullptr;
    }

    m_Device = VK_NULL_HANDLE;
}

