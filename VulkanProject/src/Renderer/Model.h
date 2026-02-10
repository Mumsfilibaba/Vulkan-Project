#pragma once
#include "ScenePrimitives.h"
#include "MathHelper.h"

class CBuffer;
class CAccelerationStructure;
class CTextureResource;
class CDevice;

struct SVertex
{
    static VkVertexInputBindingDescription* GetBindingDescription()
    {
        static VkVertexInputBindingDescription BindingDescriptions[1];

        BindingDescriptions[0].binding   = 0;
        BindingDescriptions[0].stride    = sizeof(SVertex);
        BindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescriptions;
    }
    
    static VkVertexInputAttributeDescription* GetAttributeDescriptions()
    {
        static VkVertexInputAttributeDescription AttributeDescriptions[4];

        AttributeDescriptions[0].binding  = 0;
        AttributeDescriptions[0].location = 0;
        AttributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[0].offset   = offsetof(SVertex, Position);

        AttributeDescriptions[1].binding  = 0;
        AttributeDescriptions[1].location = 1;
        AttributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[1].offset   = offsetof(SVertex, Normal);

        AttributeDescriptions[2].binding  = 0;
        AttributeDescriptions[2].location = 2;
        AttributeDescriptions[2].format   = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[2].offset   = offsetof(SVertex, Tangent);

        AttributeDescriptions[3].binding  = 0;
        AttributeDescriptions[3].location = 3;
        AttributeDescriptions[3].format   = VK_FORMAT_R32G32_SFLOAT;
        AttributeDescriptions[3].offset   = offsetof(SVertex, TexCoord);

        return AttributeDescriptions;
    }

    bool operator==(const SVertex& Other) const
    {
        return Position == Other.Position && Normal == Other.Normal && Tangent == Other.Tangent && TexCoord == Other.TexCoord;
    }

    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec3 Tangent;
    glm::vec2 TexCoord;
};

struct SVertexHasher
{
    size_t operator()(const SVertex& Vertex) const
    {
        size_t CurrentHash = std::hash<glm::vec3>()(Vertex.Position);
        Hash::Combine(CurrentHash, Vertex.Normal);
        Hash::Combine(CurrentHash, Vertex.Tangent);
        Hash::Combine(CurrentHash, Vertex.TexCoord);
        return CurrentHash;
    }
};

struct SVertexPosition
{
    static VkVertexInputBindingDescription* GetBindingDescription()
    {
        static VkVertexInputBindingDescription BindingDescriptions[1];

        BindingDescriptions[0].binding   = 0;
        BindingDescriptions[0].stride    = sizeof(SVertexPosition);
        BindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescriptions;
    }

    static VkVertexInputAttributeDescription* GetAttributeDescriptions()
    {
        static VkVertexInputAttributeDescription AttributeDescriptions[1];

        AttributeDescriptions[0].binding  = 0;
        AttributeDescriptions[0].location = 0;
        AttributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[0].offset   = offsetof(SVertexPosition, Position);

        return AttributeDescriptions;
    }

    bool operator==(const SVertexPosition& Other) const
    {
        return Position == Other.Position;
    }

    glm::vec3 Position;
};

struct SVertexPosOnlyHasher
{
    size_t operator()(const SVertexPosition& Vertex) const
    {
        return std::hash<glm::vec3>()(Vertex.Position);
    }
};

struct SMaterial
{
    std::shared_ptr<CTextureResource> AlbedoTex;
    std::shared_ptr<CTextureResource> NormalTex;
    std::shared_ptr<CTextureResource> AlphaMaskTex;
    std::shared_ptr<CTextureResource> RoughnessTex;
    std::shared_ptr<CTextureResource> MetallicTex;
};

struct SModel
{
    struct SSubMesh
    {
        SSubMesh()
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

    SModel();
    ~SModel();

    bool LoadFromFile(const std::string& filepath, CDevice* pDevice);

    CBuffer*                pVertexBuffer;
    CBuffer*                pIndexBuffer;
    uint32_t                VertexCount;
    uint32_t                IndexCount;
    CAccelerationStructure* pAccelerationStructure;

    std::vector<SSubMesh>   SubMeshes;
    std::vector<uint32_t>   Indicies;
    std::vector<SVertex>    Vertices;
    std::vector<SMaterial>  Materials;
};
