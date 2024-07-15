#include "Model.h"
#include <tiny_obj_loader.h>

FModel::FModel()
    : m_pVertexBuffer(nullptr)
    , m_pIndexBuffer(nullptr)
    , m_VertexCount(0)
    , m_IndexCount(0)
{
}

FModel::~FModel()
{
    SAFE_DELETE(m_pVertexBuffer);
    SAFE_DELETE(m_pIndexBuffer);
}

bool FModel::LoadFromFile(const std::string& Filepath, FDevice* pDevice, FDeviceMemoryAllocator* pAllocator)
{
    tinyobj::attrib_t                Attrib;
    std::vector<tinyobj::shape_t>    Shapes;
    std::vector<tinyobj::material_t> Materials;
    std::string                      Warning;
    std::string                      Error;

    if (!tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &Warning, &Error, Filepath.c_str()))
    {
        std::cout << "Failed to load model '" << Filepath << "'" << std::endl;
        if (!Warning.empty())
        {
            std::cout << "  Warning: " << Warning << std::endl;
        }
        if (!Error.empty())
        {
            std::cout << "  Error: " << Error << std::endl;
        }
        
        return false;
    }
    else
    {
        std::cout << "Loaded model '" << Filepath << "'" << std::endl;
        if (!Warning.empty())
        {
            std::cout << "  Warning: " << Warning << std::endl;
        }
    }
    
    std::vector<FVertex>  Vertices;
    std::vector<uint16_t> Indices;
    std::unordered_map<FVertex, uint16_t, FVertexHasher> UniqueVertices = {};
    for (const auto& Shape : Shapes)
    {
        for (const auto& Index : Shape.mesh.indices)
        {
            const size_t BaseIndex = 3 * Index.vertex_index;
            
            FVertex Vertex{};
            Vertex.Position =
            {
                Attrib.vertices[BaseIndex + 0],
                Attrib.vertices[BaseIndex + 1],
                Attrib.vertices[BaseIndex + 2]
            };

            Vertex.TexCoord =
            {
                Attrib.texcoords[2 * Index.texcoord_index + 0],
                1.0f - Attrib.texcoords[2 * Index.texcoord_index + 1]
            };

            if (UniqueVertices.count(Vertex) == 0)
            {
                UniqueVertices[Vertex] = static_cast<uint32_t>(Vertices.size());
                Vertices.push_back(Vertex);
            }

            Indices.push_back(UniqueVertices[Vertex]);
        }
    }
    
    assert(Indices.size() < UINT16_MAX);
    
    FBufferParams VertexBufferParams = {};
    VertexBufferParams.Size             = Vertices.size() * sizeof(FVertex);
    VertexBufferParams.Usage            = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VertexBufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;
    m_pVertexBuffer = FBuffer::Create(pDevice, VertexBufferParams, pAllocator);

    void* pCPUMem = m_pVertexBuffer->Map();
    memcpy(pCPUMem, Vertices.data(), VertexBufferParams.Size);
    m_pVertexBuffer->Unmap();

    FBufferParams IndexBufferParams = {};
    IndexBufferParams.Size             = Indices.size() * sizeof(uint16_t);
    IndexBufferParams.Usage            = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    IndexBufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;
    m_pIndexBuffer = FBuffer::Create(pDevice, IndexBufferParams, pAllocator);

    pCPUMem = m_pIndexBuffer->Map();
    memcpy(pCPUMem, Indices.data(), IndexBufferParams.Size);
    m_pIndexBuffer->Unmap();
    
    m_VertexCount = Vertices.size();
    m_IndexCount  = Indices.size();
    return true;
}

bool FMesh::LoadFromFile(const std::string& Filepath)
{
    tinyobj::attrib_t                Attrib;
    std::vector<tinyobj::shape_t>    Shapes;
    std::vector<tinyobj::material_t> Materials;
    std::string                      Warning;
    std::string                      Error;

    if (!tinyobj::LoadObj(&Attrib, &Shapes, &Materials, &Warning, &Error, Filepath.c_str()))
    {
        std::cout << "Failed to load model '" << Filepath << "'" << std::endl;
        if (!Warning.empty())
        {
            std::cout << "  Warning: " << Warning << std::endl;
        }
        if (!Error.empty())
        {
            std::cout << "  Error: " << Error << std::endl;
        }
        
        return false;
    }
    else
    {
        std::cout << "Loaded model '" << Filepath << "'" << std::endl;
        if (!Warning.empty())
        {
            std::cout << "  Warning: " << Warning << std::endl;
        }
    }
    
    std::vector<uint32_t>       NewIndices;
    std::vector<FVertexPosOnly> NewVertices;
    std::vector<FVertexEx>      NewVerticesEx;
        
    std::unordered_map<FVertex, uint32_t, FVertexHasher> UniqueVertices;
    for (const auto& Shape : Shapes)
    {
        for (const auto& Index : Shape.mesh.indices)
        {
            const size_t BasePositionIndex = 3 * Index.vertex_index;
            
            FVertex Vertex;
            Vertex.Position =
            {
                Attrib.vertices[BasePositionIndex + 0],
                Attrib.vertices[BasePositionIndex + 1],
                Attrib.vertices[BasePositionIndex + 2],
            };
            
            if (Index.normal_index >= 0)
            {
                const size_t BaseNormalIndex = 3 * Index.normal_index;
                Vertex.Normal =
                {
                    Attrib.normals[BaseNormalIndex + 0],
                    Attrib.normals[BaseNormalIndex + 1],
                    Attrib.normals[BaseNormalIndex + 2],
                };
            }
            
            if (Index.texcoord_index >= 0)
            {
                const size_t BaseTexCoordIndex = 2 * Index.texcoord_index;
                Vertex.TexCoord =
                {
                    Attrib.texcoords[BaseTexCoordIndex + 0],
                    1.0f - Attrib.texcoords[BaseTexCoordIndex + 1]
                };
            }
            
            if (UniqueVertices.count(Vertex) == 0)
            {
                UniqueVertices[Vertex] = static_cast<uint32_t>(NewVertices.size());
                
                // Convert this massive vertex into the two "lighter" vertices
                NewVertices.push_back({ glm::vec4(Vertex.Position, 0.0f) });
                NewVerticesEx.push_back({ glm::vec4(Vertex.Normal, 0.0f), glm::vec4(Vertex.TexCoord, 0.0f, 0.0f) });
            }

            NewIndices.push_back(UniqueVertices[Vertex]);
        }
    }
    
    //
    Vertices   = std::move(NewVertices);
    VerticesEx = std::move(NewVerticesEx);
    Indicies   = std::move(NewIndices);
    return true;
}
