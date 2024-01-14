#include "Model.h"
#include <tiny_obj_loader.h>

FModel::~FModel()
{
    SAFE_DELETE(m_pVertexBuffer);
    SAFE_DELETE(m_pIndexBuffer);
}

bool FModel::LoadFromFile(const std::string& filepath, FDevice* pDevice, FDeviceMemoryAllocator* pAllocator)
{
    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::string                      warning;
    std::string                      error;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, filepath.c_str()))
    {
        std::cout << "Failed to load model '" << filepath << "'" << std::endl;
        if (!warning.empty())
        {
            std::cout << "  Warning: " << warning << std::endl;
        }
        if (!error.empty())
        {
            std::cout << "  Error: " << error << std::endl;
        }
        
        return false;
    }
    else
    {
        std::cout << "Loaded model '" << filepath << "'" << std::endl;
        if (!warning.empty())
        {
            std::cout << "  Warning: " << warning << std::endl;
        }
    }
    
    std::vector<FVertex>  vertices;
    std::vector<uint16_t> indices;
    std::unordered_map<FVertex, uint16_t, FVertexHasher> uniqueVertices = {};
    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            FVertex vertex{};
            vertex.Position =
            {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.TexCoord =
            {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.Color = { 1.0f, 1.0f, 1.0f };

            if (uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }
    
    assert(indices.size() < UINT16_MAX);
    
    FBufferParams vertexBufferParams = {};
    vertexBufferParams.Size             = vertices.size() * sizeof(FVertex);
    vertexBufferParams.Usage            = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vertexBufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;
    m_pVertexBuffer = FBuffer::Create(pDevice, vertexBufferParams, pAllocator);

    void* pCPUMem = m_pVertexBuffer->Map();
    memcpy(pCPUMem, vertices.data(), vertexBufferParams.Size);
    m_pVertexBuffer->Unmap();

    FBufferParams indexBufferParams = {};
    indexBufferParams.Size             = indices.size() * sizeof(uint16_t);
    indexBufferParams.Usage            = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    indexBufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;
    m_pIndexBuffer = FBuffer::Create(pDevice, indexBufferParams, pAllocator);

    pCPUMem = m_pIndexBuffer->Map();
    memcpy(pCPUMem, indices.data(), indexBufferParams.Size);
    m_pIndexBuffer->Unmap();
    
    m_VertexCount = vertices.size();
    m_IndexCount  = indices.size();
    return true;
}
