#pragma once
#include "ScenePrimitives.h"
#include "MathHelper.h"
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
        static VkVertexInputAttributeDescription AttributeDescriptions[4];

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
        AttributeDescriptions[2].format   = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[2].offset   = offsetof(FVertex, Tangent);

        AttributeDescriptions[3].binding  = 0;
        AttributeDescriptions[3].location = 3;
        AttributeDescriptions[3].format   = VK_FORMAT_R32G32_SFLOAT;
        AttributeDescriptions[3].offset   = offsetof(FVertex, TexCoord);

        return AttributeDescriptions;
    }

    bool operator==(const FVertex& Other) const
    {
        return Position == Other.Position && Normal == Other.Normal && Tangent == Other.Tangent && TexCoord == Other.TexCoord;
    }

    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec3 Tangent;
    glm::vec2 TexCoord;
};

struct FVertexHasher
{
    size_t operator()(const FVertex& Vertex) const
    {
        size_t CurrentHash = std::hash<glm::vec3>()(Vertex.Position);
        Hash::Combine(CurrentHash, Vertex.Normal);
        Hash::Combine(CurrentHash, Vertex.Tangent);
        Hash::Combine(CurrentHash, Vertex.TexCoord);
        return CurrentHash;
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

    glm::vec3 Position;
};

struct FVertexPosOnlyHasher
{
    size_t operator()(const FVertexPosOnly& Vertex) const
    {
        return std::hash<glm::vec3>()(Vertex.Position);
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
        return std::hash<glm::vec3>()(Vertex.Position);
    }
};

struct FTriangleInfo
{
    uint32_t MaterialIndex;
};

struct FMaterial
{
    std::shared_ptr<FTextureResource> AlbedoTex;
    std::shared_ptr<FTextureResource> NormalTex;
    std::shared_ptr<FTextureResource> AlphaMaskTex;
    std::shared_ptr<FTextureResource> RoughnessTex;
    std::shared_ptr<FTextureResource> MetallicTex;
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
            , MaterialIndex(0)
        {
        }

        uint32_t IndexCount;
        uint32_t IndexOffset;
        uint32_t VertexCount;
        uint32_t VertexOffset;
        int32_t  MaterialIndex;
    };

    FModel();
    ~FModel();

    bool LoadFromFile(const std::string& filepath, FDevice* pDevice);

    FBuffer*               pVertexBuffer;
    FBuffer*               pIndexBuffer;
    uint32_t               VertexCount;
    uint32_t               IndexCount;

    std::vector<FSubMesh>  SubMeshes;
    std::vector<uint32_t>  Indicies;
    std::vector<FVertex>   Vertices;
    std::vector<FMaterial> Materials;
};

struct FMesh
{
    bool LoadFromFile(const std::string& Filepath);

    std::vector<FTriangleInfo>  TriangleInfo;
    std::vector<uint32_t>       Indicies;
    std::vector<FVertex>        VerticesEx;
    std::vector<FMaterial>      Materials;
};
