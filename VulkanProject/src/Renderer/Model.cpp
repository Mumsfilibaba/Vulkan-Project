#include "Model.h"
#include "TextureResource.h"
#include "Application.h"
#include "Vulkan/BindlessManager.h"
#include <tiny_obj_loader.h>

#pragma optimize("", off)

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
    : pVertexBuffer(nullptr)
    , pIndexBuffer(nullptr)
    , VertexCount(0)
    , IndexCount(0)
    , SubMeshes()
    , Materials()
{
}

FModel::~FModel()
{
    SAFE_DELETE(pVertexBuffer);
    SAFE_DELETE(pIndexBuffer);
}

bool FModel::LoadFromFile(const std::string& Filepath, FDevice* pDevice)
{
    tinyobj::attrib_t                TinyObjAttrib;
    std::vector<tinyobj::shape_t>    TinyObjShapes;
    std::vector<tinyobj::material_t> TinyObjMaterials;
    std::string                      TinyObjWarning;
    std::string                      TinyObjError;

    const std::string MaterialPath = ExtractPath(Filepath);
    if (!tinyobj::LoadObj(&TinyObjAttrib, &TinyObjShapes, &TinyObjMaterials, &TinyObjWarning, &TinyObjError, Filepath.c_str(), MaterialPath.c_str(), true))
    {
        LOG("Failed to load model '%s'\n", Filepath.c_str());
        if (!TinyObjWarning.empty())
        {
            LOG("  Warning: %s\n", TinyObjWarning.c_str());
        }
        if (!TinyObjError.empty())
        {
            LOG("  Error: %s\n", TinyObjError.c_str());
        }

        return false;
    }
    else
    {
        LOG("Loading model... '%s'\n", Filepath.c_str());
        if (!TinyObjWarning.empty())
        {
            LOG("Warning: %s\n", TinyObjWarning.c_str());
        }
    }

    // Parse Materials
    std::unordered_map<std::string, std::shared_ptr<FTextureResource>> MaterialTextures;
    const auto LoadMaterialTexture = [&](const std::string& TextureName)
    {
        std::shared_ptr<FTextureResource> Texture;
        if (!TextureName.empty())
        {
            auto It = MaterialTextures.find(TextureName);
            if (It == MaterialTextures.end())
            {
                std::string Path = MaterialPath + TextureName;
                ReplaceBackslashes(Path);

                Texture = std::shared_ptr<FTextureResource>(FTextureResource::LoadFromFile(pDevice, Path.c_str()));
                if (Texture)
                {
                    MaterialTextures.insert(std::make_pair(TextureName, Texture));
                }
            }
            else
            {
                Texture = It->second;
            }
        }

        return Texture;
    };

    std::vector<FMaterial> NewMaterials;
    for (const tinyobj::material_t& Material : TinyObjMaterials)
    {
        FMaterial NewMaterial;
        NewMaterial.AlbedoTex = LoadMaterialTexture(Material.diffuse_texname);
        NewMaterial.NormalTex = LoadMaterialTexture(Material.bump_texname);
        NewMaterials.emplace_back(std::move(NewMaterial));
    }

    std::vector<FSubMesh> NewSubmeshes;
    std::vector<FVertex>  NewVertices;
    std::vector<uint32_t> NewIndices;

    std::unordered_map<FVertex, uint32_t, FVertexHasher> UniqueVertices;
    for (const tinyobj::shape_t& Shape : TinyObjShapes)
    {
        FSubMesh& SubMesh = NewSubmeshes.emplace_back();
        SubMesh.VertexOffset  = NewVertices.size();
        SubMesh.IndexOffset   = NewIndices.size();
        SubMesh.MaterialIndex = Shape.mesh.material_ids[0];

        for (const tinyobj::index_t& Index : Shape.mesh.indices)
        {
            // Positions must be present
            const size_t BasePositionIndex = 3 * Index.vertex_index;
            assert(BasePositionIndex >= 0);

            FVertex Vertex;
            Vertex.Position =
            {
                TinyObjAttrib.vertices[BasePositionIndex + 0],
                TinyObjAttrib.vertices[BasePositionIndex + 1],
                TinyObjAttrib.vertices[BasePositionIndex + 2],
            };

            // Check for normals
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

            // Check for UVs
            if (Index.texcoord_index >= 0)
            {
                const size_t BaseTexCoordIndex = 2 * Index.texcoord_index;
                Vertex.TexCoord =
                {
                    TinyObjAttrib.texcoords[BaseTexCoordIndex + 0],
                    1.0f - TinyObjAttrib.texcoords[BaseTexCoordIndex + 1],
                };
            }

            if (UniqueVertices.count(Vertex) == 0)
            {
                UniqueVertices[Vertex] = static_cast<uint32_t>(NewVertices.size());
                NewVertices.push_back(Vertex);
            }

            NewIndices.push_back(UniqueVertices[Vertex]);
        }

        SubMesh.VertexCount = NewVertices.size() - SubMesh.VertexOffset;
        SubMesh.IndexCount  = NewIndices.size()  - SubMesh.IndexOffset;
    }

    // Ensure everything is correct
    const size_t TriangleCount = NewIndices.size() / 3;
    assert(TriangleCount == NewTriangleInfo.size());

    LOG("... finished loading model '%s'\n", Filepath.c_str());
    LOG("Calculating Tangents...\n");

    // Calculate tangents for each triangle
    std::vector<glm::vec3> TangentAccumulation;
    TangentAccumulation.resize(NewVertices.size());

    for (size_t i = 0; i < NewIndices.size(); i += 3)
    {
        uint32_t Index0 = NewIndices[i + 0];
        uint32_t Index1 = NewIndices[i + 1];
        uint32_t Index2 = NewIndices[i + 2];

        glm::vec3 Edge1    = NewVertices[Index1].Position - NewVertices[Index0].Position;
        glm::vec3 Edge2    = NewVertices[Index2].Position - NewVertices[Index0].Position;
        glm::vec2 DeltaUV1 = NewVertices[Index1].TexCoord - NewVertices[Index0].TexCoord;
        glm::vec2 DeltaUV2 = NewVertices[Index2].TexCoord - NewVertices[Index0].TexCoord;

        float Denom = DeltaUV1.x * DeltaUV2.y - DeltaUV2.x * DeltaUV1.y;
        float f     = std::abs(Denom) > 0.0f ? 1.0f / Denom : 0.0f;

        glm::vec3 Tangent;
        Tangent.x = f * (DeltaUV2.y * Edge1.x - DeltaUV1.y * Edge2.x);
        Tangent.y = f * (DeltaUV2.y * Edge1.y - DeltaUV1.y * Edge2.y);
        Tangent.z = f * (DeltaUV2.y * Edge1.z - DeltaUV1.y * Edge2.z);

        TangentAccumulation[Index0] += Tangent;
        TangentAccumulation[Index1] += Tangent;
        TangentAccumulation[Index2] += Tangent;
    }

    for (size_t i = 0; i < NewVertices.size(); i++)
    {
        glm::vec3 Tangent = glm::normalize(TangentAccumulation[i]);
        NewVertices[i].Tangent = glm::vec4(Tangent, 0.0);
    }

    LOG("... Finished calculating Tangents\n");

    assert(NewIndices.size() < UINT32_MAX);

    FBufferParams VertexBufferParams = { };
    VertexBufferParams.Size  = NewVertices.size() * sizeof(FVertex);
    VertexBufferParams.Usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_RAY_TRACING_INPUT;
    VertexBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;

    pVertexBuffer = FBuffer::CreateWithData(pDevice, VertexBufferParams, nullptr, NewVertices.data());
    assert(pVertexBuffer != nullptr);
    VertexCount = NewVertices.size();

    FBufferParams IndexBufferParams = { };
    IndexBufferParams.Size  = NewIndices.size() * sizeof(uint32_t);
    IndexBufferParams.Usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_RAY_TRACING_INPUT;
    IndexBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;

    pIndexBuffer = FBuffer::CreateWithData(pDevice, IndexBufferParams, nullptr, NewIndices.data());
    assert(pIndexBuffer != nullptr);
    IndexCount = NewIndices.size();

    SubMeshes = std::move(NewSubmeshes);
    Materials = std::move(NewMaterials);
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
        LOG("Failed to load model '%s'\n", Filepath.c_str());
        if (!TinyObjWarning.empty())
        {
            LOG("  Warning: %s\n", TinyObjWarning.c_str());
        }
        if (!TinyObjError.empty())
        {
            LOG("  Error: %s\n", TinyObjError.c_str());
        }

        return false;
    }
    else
    {
        LOG("Loading model... '%s'\n", Filepath.c_str());
        if (!TinyObjWarning.empty())
        {
            LOG("Warning: %s\n", TinyObjWarning.c_str());
        }
    }

    // Parse Materials
    FDevice* pDevice = FApplication::Get().GetDevice();

    std::unordered_map<std::string, std::shared_ptr<FTextureResource>> MaterialTextures;
    const auto LoadMaterialTexture = [&](const std::string& TextureName)
    {
        std::shared_ptr<FTextureResource> Texture;
        if (!TextureName.empty())
        {
            auto It = MaterialTextures.find(TextureName);
            if (It == MaterialTextures.end())
            {
                std::string Path = MaterialPath + TextureName;
                ReplaceBackslashes(Path);

                Texture = std::shared_ptr<FTextureResource>(FTextureResource::LoadFromFile(pDevice, Path.c_str()));
                if (Texture)
                {
                    MaterialTextures.insert(std::make_pair(TextureName, Texture));
                }
            }
            else
            {
                Texture = It->second;
            }
        }

        return Texture;
    };

    std::vector<FMaterial> NewMaterials;
    for (const tinyobj::material_t& Material : TinyObjMaterials)
    {
        FMaterial NewMaterial;
        NewMaterial.AlbedoTex = LoadMaterialTexture(Material.diffuse_texname);
        NewMaterial.NormalTex = LoadMaterialTexture(Material.bump_texname);
        NewMaterials.emplace_back(std::move(NewMaterial));
    }

    // Parse the vertices
    std::vector<FTriangleInfo>  NewTriangleInfo;
    std::vector<uint32_t>       NewIndices;
    std::vector<FVertexPosOnly> NewVertices;
    std::vector<FVertexEx>      NewVerticesEx;
    std::vector<uint32_t>       MaterialIndicies;

    std::unordered_map<FVertex, uint32_t, FVertexHasher> UniqueVertices;
    for (const tinyobj::shape_t& Shape : TinyObjShapes)
    {
        size_t CurrentIndex      = 0;
        size_t VerticesProcessed = 0;
        for (const tinyobj::index_t& Index : Shape.mesh.indices)
        {
            FVertex Vertex;

            // Positions must be present
            const size_t BasePositionIndex = 3 * Index.vertex_index;
            assert(BasePositionIndex >= 0);

            Vertex.Position =
            {
                TinyObjAttrib.vertices[BasePositionIndex + 0],
                TinyObjAttrib.vertices[BasePositionIndex + 1],
                TinyObjAttrib.vertices[BasePositionIndex + 2],
            };

            // Check for normals
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

            // Check for UVs
            if (Index.texcoord_index >= 0)
            {
                const size_t BaseTexCoordIndex = 2 * Index.texcoord_index;
                Vertex.TexCoord =
                {
                    TinyObjAttrib.texcoords[BaseTexCoordIndex + 0],
                    1.0f - TinyObjAttrib.texcoords[BaseTexCoordIndex + 1]
                };
            }

            // Add unique vertex data
            if (UniqueVertices.count(Vertex) == 0)
            {
                UniqueVertices[Vertex] = static_cast<uint32_t>(NewVertices.size());

                // Convert this massive vertex into the two "lighter" vertices
                FVertexPosOnly& NewVertex = NewVertices.emplace_back();
                NewVertex.Position = glm::vec4(Vertex.Position, 0.0f);

                FVertexEx& NewVertexEx = NewVerticesEx.emplace_back();
                NewVertexEx.Normal    = glm::vec4(Vertex.Normal, 0.0f);
                NewVertexEx.TexCoords = glm::vec4(Vertex.TexCoord, 0.0f, 0.0f);
            }

            NewIndices.push_back(UniqueVertices[Vertex]);

            // Add a new triangle
            VerticesProcessed++;

            if (VerticesProcessed >= 3)
            {
                const size_t TriangleIndex = CurrentIndex / 3;
                assert(TriangleIndex < Shape.mesh.material_ids.size());

                const int32_t MaterialIndex = Shape.mesh.material_ids[TriangleIndex];
                if (MaterialIndex >= 0)
                {
                    assert(MaterialIndex < NewMaterials.size());
                }
                
                NewTriangleInfo.push_back({ static_cast<uint32_t>(MaterialIndex) });
                VerticesProcessed = 0;
            }

            // Increment the current index (VertexIndex)
            CurrentIndex++;
        }
    }

    // Ensure everything is correct
    const size_t TriangleCount = NewIndices.size() / 3;
    assert(TriangleCount == NewTriangleInfo.size());

    LOG("... finished loading model '%s'\n", Filepath.c_str());
    LOG("Calculating Tangents...\n");

    // Calculate tangents for each triangle
    std::vector<glm::vec3> TangentAccumulation;
    TangentAccumulation.resize(NewVertices.size());

    for (size_t i = 0; i < NewIndices.size(); i += 3)
    {
        uint32_t i0 = NewIndices[i + 0];
        uint32_t i1 = NewIndices[i + 1];
        uint32_t i2 = NewIndices[i + 2];

        glm::vec3 edge1 = NewVertices[i1].Position - NewVertices[i0].Position;
        glm::vec3 edge2 = NewVertices[i2].Position - NewVertices[i0].Position;

        glm::vec2 deltaUV1 = NewVerticesEx[i1].TexCoords - NewVerticesEx[i0].TexCoords;
        glm::vec2 deltaUV2 = NewVerticesEx[i2].TexCoords - NewVerticesEx[i0].TexCoords;

        float denom = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
        float f     = std::abs(denom) > 0.0f ? 1.0f / denom : 0.0f;

        glm::vec3 tangent;
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        TangentAccumulation[i0] += tangent;
        TangentAccumulation[i1] += tangent;
        TangentAccumulation[i2] += tangent;
    }

    for (size_t i = 0; i < NewVerticesEx.size(); i++)
    {
        glm::vec3 Tangent = glm::normalize(TangentAccumulation[i]);
        NewVerticesEx[i].Tangent = glm::vec4(Tangent, 0.0);
    }

    LOG("... Finished calculating Tangents\n");

    // Setup the vertices
    TriangleInfo = std::move(NewTriangleInfo);
    Vertices     = std::move(NewVertices);
    VerticesEx   = std::move(NewVerticesEx);
    Indicies     = std::move(NewIndices);
    Materials    = std::move(NewMaterials);
    return true;
}
