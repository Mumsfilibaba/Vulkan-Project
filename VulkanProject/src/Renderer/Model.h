#pragma once
#include "Vulkan/Buffer.h"
#include "Vulkan/Device.h"
#include "Vulkan/DeviceMemoryAllocator.h"

struct FVertex
{
    glm::vec3 Position;
    glm::vec2 TexCoord;
    glm::vec3 Color;
    
    bool operator==(const FVertex& other) const
    {
        return
            Position == other.Position &&
            TexCoord == other.TexCoord &&
            Color == other.Color;
    }

    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding   = 0;
        bindingDescription.stride    = sizeof(FVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static VkVertexInputAttributeDescription* GetAttributeDescriptions()
    {
        static VkVertexInputAttributeDescription attributeDescriptions[3];

        attributeDescriptions[0].binding    = 0;
        attributeDescriptions[0].location   = 0;
        attributeDescriptions[0].format     = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset     = offsetof(FVertex, Position);
        
        attributeDescriptions[1].binding    = 0;
        attributeDescriptions[1].location   = 1;
        attributeDescriptions[1].format     = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset     = offsetof(FVertex, TexCoord);

        attributeDescriptions[2].binding    = 0;
        attributeDescriptions[2].location   = 2;
        attributeDescriptions[2].format     = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset     = offsetof(FVertex, Color);

        return attributeDescriptions;
    }
};

struct FVertexHasher
{
    size_t operator()(const FVertex& vertex) const
    {
        using namespace std;
        return ((hash<glm::vec3>()(vertex.Position) ^ (hash<glm::vec3>()(vertex.Color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.TexCoord) << 1);
    }
};

class FModel
{
public:
    FModel() = default;
    ~FModel();
    
    bool LoadFromFile(const std::string& filepath, FDevice* pDevice, FDeviceMemoryAllocator* pAllocator);
    
    inline FBuffer* GetVertexBuffer() const
    {
        return m_pVertexBuffer;
    }
    
    inline FBuffer* GetIndexBuffer() const
    {
        return m_pIndexBuffer;
    }
    
    inline uint32_t GetVertexCount() const
    {
        return m_VertexCount;
    }
    
    inline uint32_t GetIndexCount() const
    {
        return m_IndexCount;
    }
    
private:
    FBuffer* m_pVertexBuffer = nullptr;
    FBuffer* m_pIndexBuffer = nullptr;
    uint32_t m_VertexCount = 0;
    uint32_t m_IndexCount = 0;
};
