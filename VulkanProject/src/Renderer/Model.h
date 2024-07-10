#pragma once
#include "Vulkan/Buffer.h"
#include "Vulkan/Device.h"
#include "Vulkan/DeviceMemoryAllocator.h"

struct FVertex
{
    static VkVertexInputBindingDescription* GetBindingDescription()
    {
        static VkVertexInputBindingDescription BindingDescriptions[1];
        
        BindingDescriptions[0].binding   = 0;
        BindingDescriptions[0].stride    = sizeof(FVertex);
        BindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        
        return BindingDescriptions;
    }
    
    static VkVertexInputAttributeDescription* GetAttributeDescriptions()
    {
        static VkVertexInputAttributeDescription AttributeDescriptions[3];
        
        AttributeDescriptions[0].binding  = 0;
        AttributeDescriptions[0].location = 0;
        AttributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[0].offset   = offsetof(FVertex, Position);
        
        AttributeDescriptions[1].binding  = 0;
        AttributeDescriptions[1].location = 1;
        AttributeDescriptions[1].format   = VK_FORMAT_R32G32_SFLOAT;
        AttributeDescriptions[1].offset   = offsetof(FVertex, TexCoord);
        
        AttributeDescriptions[2].binding  = 0;
        AttributeDescriptions[2].location = 2;
        AttributeDescriptions[2].format   = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[2].offset   = offsetof(FVertex, Color);
        
        return AttributeDescriptions;
    }

    bool operator==(const FVertex& Other) const
    {
        return Position == Other.Position && TexCoord == Other.TexCoord && Color == Other.Color;
    }
    
    glm::vec3 Position;
    glm::vec2 TexCoord;
    glm::vec3 Color;
};

struct FVertexHasher
{
    size_t operator()(const FVertex& vertex) const
    {
        using namespace std;
        return ((hash<glm::vec3>()(vertex.Position) ^ (hash<glm::vec3>()(vertex.Color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.TexCoord) << 1);
    }
};

struct FVertexPosOnly
{
    static VkVertexInputBindingDescription* GetBindingDescription()
    {
        static VkVertexInputBindingDescription BindingDescriptions[1];
        
        BindingDescriptions[0].binding   = 0;
        BindingDescriptions[0].stride    = sizeof(FVertexPosOnly);
        BindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        
        return BindingDescriptions;
    }
    
    static VkVertexInputAttributeDescription* GetAttributeDescriptions()
    {
        static VkVertexInputAttributeDescription AttributeDescriptions[1];
        
        AttributeDescriptions[0].binding  = 0;
        AttributeDescriptions[0].location = 0;
        AttributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[0].offset   = offsetof(FVertexPosOnly, Position);

        return AttributeDescriptions;
    }
    
    bool operator==(const FVertexPosOnly& Other) const
    {
        return Position == Other.Position;
    }

    glm::vec4 Position;
};

struct FVertexPosOnlyHasher
{
    size_t operator()(const FVertexPosOnly& Vertex) const
    {
        using namespace std;
        return hash<glm::vec3>()(Vertex.Position);
    }
};

class FModel
{
public:
    FModel();
    ~FModel();
    
    bool LoadFromFile(const std::string& filepath, FDevice* pDevice, FDeviceMemoryAllocator* pAllocator);
    
    FBuffer* GetVertexBuffer() const
    {
        return m_pVertexBuffer;
    }
    
    FBuffer* GetIndexBuffer() const
    {
        return m_pIndexBuffer;
    }
    
    uint32_t GetVertexCount() const
    {
        return m_VertexCount;
    }
    
    uint32_t GetIndexCount() const
    {
        return m_IndexCount;
    }
    
private:
    FBuffer* m_pVertexBuffer;
    FBuffer* m_pIndexBuffer;
    uint32_t m_VertexCount;
    uint32_t m_IndexCount;
};

struct FTriangle
{
    glm::vec3 Center;
    glm::vec3 Positions[3];
    uint32_t  Indicies[3];
};

struct FMesh
{
    bool LoadFromFile(const std::string& Filepath);
    
    std::vector<uint32_t>       Indicies;
    std::vector<FVertexPosOnly> Positions;
    glm::vec3 BoundingBoxMin;
    glm::vec3 BoundingBoxMax;
};
