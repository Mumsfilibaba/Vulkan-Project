#include "Model.h"
#include "TextureResource.h"
#include "Application.h"
#include "Vulkan/BindlessManager.h"
#include <tiny_obj_loader.h>

static std::string ExtractPath(const std::string& Path)
{
    const size_t Position = Path.find_last_of("/\\");
    if (Position != std::string::npos)
    {
        return Path.substr(0, Position + 1);
    }
    
    return Path;
}

static void ReplaceBackslashes(std::string& Path)
{
    for (char& Char : Path)
    {
        if (Char == '\\')
        {
            Char = '/';
        }
    }
}

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
    tinyobj::attrib_t                TinyObjAttrib;
    std::vector<tinyobj::shape_t>    TinyObjShapes;
    std::vector<tinyobj::material_t> TinyObjMaterials;
    std::string                      TinyObjWarning;
    std::string                      TinyObjError;

    const std::string MaterialPath = ExtractPath(Filepath);
    if (!tinyobj::LoadObj(&TinyObjAttrib, &TinyObjShapes, &TinyObjMaterials, &TinyObjWarning, &TinyObjError, Filepath.c_str(), MaterialPath.c_str(), true))
    {
        std::cout << "Failed to load model '" << Filepath << "'" << std::endl;
        if (!TinyObjWarning.empty())
        {
            std::cout << "  Warning: " << TinyObjWarning << std::endl;
        }
        if (!TinyObjError.empty())
        {
            std::cout << "  Error: " << TinyObjError << std::endl;
        }
        
        return false;
    }
    else
    {
        std::cout << "Loaded model '" << Filepath << "'" << std::endl;
        if (!TinyObjWarning.empty())
        {
            std::cout << "Warning:\n" << TinyObjWarning << std::endl;
        }
    }
    
    // Parse Materials
    FDevice* pDevice = FApplication::Get().GetDevice();
    
    std::vector<FMaterial> NewMaterials;
    std::unordered_map<std::string, std::shared_ptr<FTextureResource>> MaterialTextures;
    for (const auto& Material : TinyObjMaterials)
    {
        std::shared_ptr<FTextureResource> Texture;
        if (!Material.diffuse_texname.empty())
        {
            auto It = MaterialTextures.find(Material.diffuse_texname);
            if (It == MaterialTextures.end())
            {
                std::string Path = MaterialPath + Material.diffuse_texname;
                ReplaceBackslashes(Path);
                
                Texture = std::shared_ptr<FTextureResource>(FTextureResource::LoadFromFile(pDevice, Path.c_str()));
                if (Texture)
                {
                    MaterialTextures.insert(std::make_pair(Material.diffuse_texname, Texture));
                }
            }
            else
            {
                Texture = It->second;
            }
        }

        FMaterial NewMaterial = { Texture };
        NewMaterials.emplace_back(std::move(NewMaterial));
    }
    
    // Parse the vertices
    std::vector<uint32_t>       NewIndices;
    std::vector<FVertexPosOnly> NewVertices;
    std::vector<FVertexEx>      NewVerticesEx;
        
    std::unordered_map<FVertex, uint32_t, FVertexHasher> UniqueVertices;
    for (const auto& Shape : TinyObjShapes)
    {
        for (const auto& Index : Shape.mesh.indices)
        {
            const size_t BasePositionIndex = 3 * Index.vertex_index;
            
            FVertex Vertex;
            Vertex.Position =
            {
                TinyObjAttrib.vertices[BasePositionIndex + 0],
                TinyObjAttrib.vertices[BasePositionIndex + 1],
                TinyObjAttrib.vertices[BasePositionIndex + 2],
            };
            
            if (Index.normal_index >= 0)
            {
                const size_t BaseNormalIndex = 3 * Index.normal_index;
                Vertex.Normal =
                {
                    TinyObjAttrib.normals[BaseNormalIndex + 0],
                    TinyObjAttrib.normals[BaseNormalIndex + 1],
                    TinyObjAttrib.normals[BaseNormalIndex + 2],
                };
            }
            
            if (Index.texcoord_index >= 0)
            {
                const size_t BaseTexCoordIndex = 2 * Index.texcoord_index;
                Vertex.TexCoord =
                {
                    TinyObjAttrib.texcoords[BaseTexCoordIndex + 0],
                    1.0f - TinyObjAttrib.texcoords[BaseTexCoordIndex + 1]
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
    
    // Setup the vertices
    Vertices   = std::move(NewVertices);
    VerticesEx = std::move(NewVerticesEx);
    Indicies   = std::move(NewIndices);
    Materials  = std::move(NewMaterials);
    return true;
}
