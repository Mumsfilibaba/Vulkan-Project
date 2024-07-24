#include "ShaderModule.h"
#include "Device.h"
#include "Extensions.h"

FShaderModule* FShaderModule::Create(FDevice* pDevice, const uint32_t* pByteCode, uint32_t ByteCodeLength, const char* pEntryPoint)
{
    FShaderModule* pShader = new FShaderModule(pDevice);
    assert(pEntryPoint != nullptr);
    assert(pByteCode != nullptr);
    assert(ByteCodeLength != 0);

    VkShaderModuleCreateInfo ShaderModuleCreateInfo;
    ZERO_STRUCT(&ShaderModuleCreateInfo);
    
    ShaderModuleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ShaderModuleCreateInfo.codeSize = ByteCodeLength;
    ShaderModuleCreateInfo.pCode    = pByteCode;

    VkResult Result = vkCreateShaderModule(pDevice->GetDevice(), &ShaderModuleCreateInfo, nullptr, &pShader->m_Module);
    if (Result != VK_SUCCESS)
    {
        LOG("vkCreateShaderModule failed\n");
        return nullptr;
    }
    else
    {
        const size_t Length = strlen(pEntryPoint);
        pShader->m_pEntryPoint = new char[Length + 1];
        strcpy(pShader->m_pEntryPoint, pEntryPoint);

        LOG("Created ShaderModule\n");
        return pShader;
    }
}

FShaderModule* FShaderModule::CreateFromFile(FDevice* pDevice, const char* pEntryPoint, const char* pFilePath)
{
    if (!pFilePath)
    {
        LOG("Not a valid filename\n");
        return nullptr;
    }

    std::string Filepath = std::string(pFilePath);
    std::ifstream FileStream(Filepath, std::ios::ate | std::ios::binary);
    if (FileStream.is_open()) 
    {
        size_t fileSize = (size_t)FileStream.tellg();
        std::vector<char> buffer(fileSize);

        FileStream.seekg(0);
        FileStream.read(buffer.data(), fileSize);
        FileStream.close();
                
        FShaderModule* pShader = FShaderModule::Create(pDevice, reinterpret_cast<const uint32_t*>(buffer.data()), buffer.size(), pEntryPoint);
        if (!pShader)
        {
            return nullptr;
        }
        
        LOG("Loaded Shader '%s'\n", Filepath.c_str());
        return pShader;
    }
    else
    {
        LOG("Failed to open file '%s'\n", Filepath.c_str());
        return nullptr;
    }
}

FShaderModule::FShaderModule(FDevice* pDevice)
    : FDeviceChild(pDevice)
    , m_Module(VK_NULL_HANDLE)
    , m_pEntryPoint(nullptr)
{
}

FShaderModule::~FShaderModule()
{
    if (m_Module)
    {
        vkDestroyShaderModule(GetDevice()->GetDevice(), m_Module, nullptr);
        m_Module = VK_NULL_HANDLE;
    }

    if (m_pEntryPoint)
    {
        delete m_pEntryPoint;
        m_pEntryPoint = nullptr;
    }
}

void FShaderModule::SetDebugName(const char* DebugName)
{
    if (FExtensions::vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT DebugNameInfo;
        ZERO_STRUCT(&DebugNameInfo);
        
        DebugNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        DebugNameInfo.objectType   = VK_OBJECT_TYPE_SHADER_MODULE;
        DebugNameInfo.pObjectName  = DebugName;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_Module);

        VkResult Result = FExtensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            LOG("Failed to set name '%s'. Error: %d\n", DebugNameInfo.pObjectName, Result);
        }
    }
}
