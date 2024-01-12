#include "ShaderModule.h"
#include "Device.h"
#include <fstream>
#include <iostream>

FShaderModule* FShaderModule::Create(FDevice* pDevice, const uint32_t* pByteCode, uint32_t byteCodeLength, const char* pEntryPoint)
{
    FShaderModule* pShader = new FShaderModule(pDevice->GetDevice());
    assert(pEntryPoint != nullptr);
    assert(pByteCode != nullptr);
    assert(byteCodeLength != 0);

    VkShaderModuleCreateInfo createInfo;
    ZERO_STRUCT(&createInfo);
    
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = byteCodeLength;
    createInfo.pCode    = pByteCode;

    VkResult result = vkCreateShaderModule(pDevice->GetDevice(), &createInfo, nullptr, &pShader->m_Module);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkCreateShaderModule failed\n";
        return nullptr;
    }
    else
    {
        const size_t len = strlen(pEntryPoint);
        pShader->m_pEntryPoint = new char[len + 1];
        strcpy(pShader->m_pEntryPoint, pEntryPoint);

        std::cout << "Created ShaderModule\n";
        return pShader;
    }
}

FShaderModule* FShaderModule::CreateFromFile(FDevice* pDevice, const char* pEntryPoint, const char* pFilePath)
{
    if (!pFilePath)
    {
        std::cout << "Not a valid filename\n";
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
                
        FShaderModule* newShader = FShaderModule::Create(pDevice, reinterpret_cast<const uint32_t*>(buffer.data()), buffer.size(), pEntryPoint);
        if (!newShader)
        {
            return nullptr;
        }
        
        std::cout << "Loaded Shader '" << filepath << "'\n";
        return newShader;
    }
    else
    {
        std::cout << "Failed to open file '" << filepath << "'\n";
        return nullptr;
    }
}

FShaderModule::FShaderModule(VkDevice device)
    : m_Device(device)
    , m_Module(VK_NULL_HANDLE)
    , m_pEntryPoint(nullptr)
{
}

FShaderModule::~FShaderModule()
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

