#include "VulkanShaderModule.h"
#include "VulkanContext.h"

#include <fstream>
#include <iostream>

VulkanShaderModule::VulkanShaderModule(VkDevice device, const ShaderModuleParams& params)
    : m_Device(device),
    m_Module(VK_NULL_HANDLE),
    m_pEntryPoint(nullptr)
{
    assert(params.pSource);
    assert(params.pEntryPoint);

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext    = nullptr;
    createInfo.flags    = 0;
    createInfo.codeSize = params.SourceSize;
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(params.pSource);

    VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &m_Module);
    if (result != VK_SUCCESS) 
    {
        std::cout << "vkCreateShaderModule failed" << std::endl;
    }
    else
    {
        size_t len = strlen(params.pEntryPoint);
        m_pEntryPoint = new char[len + 1];

        strcpy(m_pEntryPoint, params.pEntryPoint);

        std::cout << "Created ShaderModule" << std::endl;
    }
}

VulkanShaderModule::~VulkanShaderModule()
{
    if (m_Module)
    {
        vkDestroyShaderModule(m_Device, m_Module, nullptr);
        m_Module = VK_NULL_HANDLE;

        std::cout << "Destroyed ShaderModule" << std::endl;
    }

    if (m_pEntryPoint)
    {
        delete m_pEntryPoint;
        m_pEntryPoint = nullptr;
    }
}

VulkanShaderModule* VulkanShaderModule::CreateFromFile(VulkanContext* pContext, const char* pEntryPoint, const char* pFilePath)
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

        ShaderModuleParams params = {};
        params.pEntryPoint 	= pEntryPoint;
        params.pSource 		= buffer.data();
        params.SourceSize 	= uint32_t(buffer.size());
		
		VulkanShaderModule* pShader = pContext->CreateShaderModule(params);
        if (pShader)
		{
			std::cout << "Loaded Shader '" << filepath << "'" << std::endl;
		}
		
		return pShader;
    }
    else
    {
        std::cout << "Failed to open file '" << filepath << "'" << std::endl;
        return nullptr;
    }
}
