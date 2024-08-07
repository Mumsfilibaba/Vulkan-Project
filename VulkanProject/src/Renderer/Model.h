#pragma once
#include "ScenePrimitives.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/Device.h"
#include "Vulkan/DeviceMemoryAllocator.h"

class FTextureResource;

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
        AttributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[1].offset   = offsetof(FVertex, Normal);
        
        AttributeDescriptions[2].binding  = 0;
        AttributeDescriptions[2].location = 2;
        AttributeDescriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
        AttributeDescriptions[2].offset   = offsetof(FVertex, TexCoord);
        
        return AttributeDescriptions;
    }

    bool operator==(const FVertex& Other) const
    {
        return Position == Other.Position && Normal == Other.Normal && TexCoord == Other.TexCoord;
    }
    
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
};

struct FVertexHasher
{
    size_t operator()(const FVertex& vertex) const
    {
        using namespace std;
        return ((hash<glm::vec3>()(vertex.Position) ^ (hash<glm::vec3>()(vertex.Normal) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.TexCoord) << 1);
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
        return hash<glm::vec4>()(Vertex.Position);
    }
};

struct FVertexAABB
{
    static VkVertexInputBindingDescription* GetBindingDescription()
    {
        static VkVertexInputBindingDescription BindingDescriptions[1];
        
        BindingDescriptions[0].binding   = 0;
        BindingDescriptions[0].stride    = sizeof(FVertexAABB);
        BindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        
        return BindingDescriptions;
    }
    
    static VkVertexInputAttributeDescription* GetAttributeDescriptions()
    {
        static VkVertexInputAttributeDescription AttributeDescriptions[1];
        
        AttributeDescriptions[0].binding  = 0;
        AttributeDescriptions[0].location = 0;
        AttributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[0].offset   = offsetof(FVertexAABB, Position);

        return AttributeDescriptions;
    }
    
    bool operator==(const FVertexAABB& Other) const
    {
        return Position == Other.Position;
    }

    glm::vec3 Position;
};

struct FVertexAABBHasher
{
    size_t operator()(const FVertexAABB& Vertex) const
    {
        using namespace std;
        return hash<glm::vec3>()(Vertex.Position);
    }
};

struct FMaterial
{
    std::shared_ptr<FTextureResource> AlbedoTex;
    std::shared_ptr<FTextureResource> NormalTex;
};

struct FModel
{
    struct FSubMesh
    {
        FSubMesh()
            : IndexCount(0)
            , IndexOffset(0)
            , VertexCount(0)
            , VertexOffset(0)
        {
        }

        uint32_t IndexCount;
        uint32_t IndexOffset;
        uint32_t VertexCount;
        uint32_t VertexOffset;
    };

    FModel();
    ~FModel();

    bool LoadFromFile(const std::string& filepath, FDevice* pDevice);

    FBuffer*               pVertexBuffer;
    FBuffer*               pIndexBuffer;
    uint32_t               VertexCount;
    uint32_t               IndexCount;
    std::vector<FSubMesh>  SubMeshes;
    std::vector<FMaterial> Materials;
};

struct FVertexEx
{
    glm::vec4 Normal;
    glm::vec4 Tangent;
    glm::vec4 TexCoords;
};

struct FTriangleInfo
{
    uint32_t MaterialIndex;
};

struct FMesh
{
    bool LoadFromFile(const std::string& Filepath);
    
    std::vector<FTriangleInfo>  TriangleInfo;
    std::vector<uint32_t>       Indicies;
    std::vector<FVertexPosOnly> Vertices;
    std::vector<FVertexEx>      VerticesEx;
    std::vector<FMaterial>      Materials;
};
