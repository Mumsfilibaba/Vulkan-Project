#include "Model.h"
#include "TextureResource.h"
#include "Application.h"
#include "Vulkan/BindlessManager.h"
#include "Vulkan/AccelerationStructure.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/Device.h"
#include <tiny_obj_loader.h>

struct SVec3Hasher
{
    size_t operator()(const glm::vec3& Value) const
    {
        return Hash::Value(Value);
    }
};

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

SModel::SModel()
    : pVertexBuffer(nullptr)
    , pIndexBuffer(nullptr)
    , pAccelerationStructure(nullptr)
    , VertexCount(0)
    , IndexCount(0)
    , SubMeshes()
    , Materials()
{
}

SModel::~SModel()
{
    SAFE_DELETE(pVertexBuffer);
    SAFE_DELETE(pIndexBuffer);
    SAFE_DELETE(pAccelerationStructure);
}

bool SModel::LoadFromFile(const std::string& Filepath, CDevice* pDevice, bool bGenerateSmoothNormals)
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
    std::unordered_map<std::string, std::shared_ptr<CTextureResource>> MaterialTextures;
    const auto LoadMaterialTexture = [&](const std::string& TextureName)
    {
        std::shared_ptr<CTextureResource> Texture;
        if (!TextureName.empty())
        {
            auto It = MaterialTextures.find(TextureName);
            if (It == MaterialTextures.end())
            {
                std::string Path = MaterialPath + TextureName;
                ReplaceBackslashes(Path);

                Texture = std::shared_ptr<CTextureResource>(CTextureResource::LoadFromFile(pDevice, Path.c_str()));
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

    std::vector<SMaterial> NewMaterials;
    for (const tinyobj::material_t& Material : TinyObjMaterials)
    {
        SMaterial NewMaterial;
        NewMaterial.AlbedoTex    = LoadMaterialTexture(Material.diffuse_texname);
        NewMaterial.NormalTex    = LoadMaterialTexture(Material.bump_texname);
        NewMaterial.RoughnessTex = LoadMaterialTexture(Material.specular_highlight_texname);
        NewMaterial.AlphaMaskTex = LoadMaterialTexture(Material.alpha_texname);
        NewMaterial.MetallicTex  = LoadMaterialTexture(Material.ambient_texname);
        NewMaterials.emplace_back(std::move(NewMaterial));
    }

    // Parse Vertices
    std::vector<SSubMesh> NewSubmeshes;
    std::vector<SVertex>  NewVertices;
    std::vector<uint32_t> NewIndices;

    std::unordered_map<SVertex, uint32_t, SVertexHasher> UniqueVertices;
    std::vector<bool> VertexImportedNormals;

    bool bModelHasMissingNormals = false;
    for (const tinyobj::shape_t& Shape : TinyObjShapes)
    {
        // Start at index zero for each mesh and loop until all indices are processed
        uint32_t CurrentIndex = 0;

        const uint32_t MeshIndexCount = static_cast<uint32_t>(Shape.mesh.indices.size());
        while (CurrentIndex < MeshIndexCount)
        {
            const int32_t TriangleIndex        = CurrentIndex / 3;
            const int32_t CurrentMaterialIndex = Shape.mesh.material_ids[TriangleIndex];

            SSubMesh& SubMesh = NewSubmeshes.emplace_back();
            SubMesh.VertexOffset  = NewVertices.size();
            SubMesh.IndexOffset   = NewIndices.size();
            SubMesh.MaterialIndex = CurrentMaterialIndex;

            for (; CurrentIndex < MeshIndexCount; ++CurrentIndex)
            {
                // Break if material is not the same
                const int32_t CurrentTriangleIndex = CurrentIndex / 3;
                if (Shape.mesh.material_ids[CurrentTriangleIndex] != CurrentMaterialIndex)
                {
                    break;
                }

                // Current Index
                const tinyobj::index_t& Index = Shape.mesh.indices[CurrentIndex];

                // Positions must be present
                const size_t BasePositionIndex = 3 * Index.vertex_index;
                assert((BasePositionIndex + 2) < TinyObjAttrib.vertices.size());

                SVertex Vertex = {};
                
                // Position
                Vertex.Position =
                {
                    TinyObjAttrib.vertices[BasePositionIndex + 0],
                    TinyObjAttrib.vertices[BasePositionIndex + 1],
                    TinyObjAttrib.vertices[BasePositionIndex + 2],
                };

                // Normals
                const bool bHasImportedNormal = (Index.normal_index >= 0);
                if (bHasImportedNormal)
                {
                    const size_t BaseNormalIndex = 3 * Index.normal_index;
                    Vertex.Normal =
                    {
                        TinyObjAttrib.normals[BaseNormalIndex + 0],
                        TinyObjAttrib.normals[BaseNormalIndex + 1],
                        TinyObjAttrib.normals[BaseNormalIndex + 2],
                    };
                }
                else
                {
                    bModelHasMissingNormals = true;
                    Vertex.Normal = glm::vec3(0.0f);
                }

                // Tangent
                Vertex.Tangent = glm::vec3(0.0f, 0.0f, 0.0f);

                // Texcoords
                if (Index.texcoord_index >= 0)
                {
                    const size_t BaseTexCoordIndex = 2 * Index.texcoord_index;
                    Vertex.TexCoord =
                    {
                        TinyObjAttrib.texcoords[BaseTexCoordIndex + 0],
                        1.0f - TinyObjAttrib.texcoords[BaseTexCoordIndex + 1],
                    };
                }
                else
                {
                    Vertex.TexCoord = glm::vec2(0.0f, 0.0f);
                }

                auto UniqueIt = UniqueVertices.find(Vertex);
                if (UniqueIt == UniqueVertices.end())
                {
                    const uint32_t NewVertexIndex = static_cast<uint32_t>(NewVertices.size());
                    UniqueVertices[Vertex] = NewVertexIndex;

                    NewVertices.push_back(Vertex);
                    NewIndices.push_back(NewVertexIndex);
                    VertexImportedNormals.push_back(bHasImportedNormal);
                }
                else
                {
                    NewIndices.push_back(UniqueIt->second);
                }
            }

            SubMesh.VertexCount = NewVertices.size() - SubMesh.VertexOffset;
            SubMesh.IndexCount  = NewIndices.size()  - SubMesh.IndexOffset;

            // Ensure that there a re a valid triangle-count
            const size_t TriangleCount = SubMesh.IndexCount / 3;
            assert((SubMesh.IndexCount % 3) == 0);
        }
    }

    // Ensure everything is correct
    const size_t TriangleCount = NewIndices.size() / 3;
    assert((NewIndices.size() % 3) == 0);

    // Optional smooth-normal generation (kept behind explicit flag).
    if (bGenerateSmoothNormals)
    {
        LOG("Generating smooth vertex normals for model '%s'...\n", Filepath.c_str());

        std::vector<glm::vec3> NormalAccumulation(NewVertices.size(), glm::vec3(0.0f));
        std::unordered_map<glm::vec3, glm::vec3, SVec3Hasher> PositionNormalAccumulation;

        for (size_t i = 0; i < NewIndices.size(); i += 3)
        {
            const uint32_t Index0 = NewIndices[i + 0];
            const uint32_t Index1 = NewIndices[i + 1];
            const uint32_t Index2 = NewIndices[i + 2];

            const glm::vec3& Position0 = NewVertices[Index0].Position;
            const glm::vec3& Position1 = NewVertices[Index1].Position;
            const glm::vec3& Position2 = NewVertices[Index2].Position;

            const glm::vec3 FaceNormal = glm::cross(Position1 - Position0, Position2 - Position0);
            if (glm::dot(FaceNormal, FaceNormal) > 0.0f)
            {
                NormalAccumulation[Index0] += FaceNormal;
                NormalAccumulation[Index1] += FaceNormal;
                NormalAccumulation[Index2] += FaceNormal;

                PositionNormalAccumulation[Position0] += FaceNormal;
                PositionNormalAccumulation[Position1] += FaceNormal;
                PositionNormalAccumulation[Position2] += FaceNormal;
            }
        }

        for (size_t i = 0; i < NewVertices.size(); i++)
        {
            glm::vec3 SmoothedNormal = NormalAccumulation[i];
            auto It = PositionNormalAccumulation.find(NewVertices[i].Position);
            if (It != PositionNormalAccumulation.end())
            {
                SmoothedNormal = It->second;
            }

            if (glm::dot(SmoothedNormal, SmoothedNormal) > 0.0f)
            {
                NewVertices[i].Normal = glm::normalize(SmoothedNormal);
            }
        }
    }
    else if (bModelHasMissingNormals)
    {
        LOG("Model '%s' has missing normals. Generating face normals for missing vertices...\n", Filepath.c_str());

        std::vector<uint32_t> VertexUseCount(NewVertices.size(), 0u);
        for (uint32_t Index : NewIndices)
        {
            VertexUseCount[Index]++;
        }

        for (size_t i = 0; i < NewIndices.size(); i += 3)
        {
            const uint32_t Index0 = NewIndices[i + 0];
            const uint32_t Index1 = NewIndices[i + 1];
            const uint32_t Index2 = NewIndices[i + 2];

            const glm::vec3& Position0 = NewVertices[Index0].Position;
            const glm::vec3& Position1 = NewVertices[Index1].Position;
            const glm::vec3& Position2 = NewVertices[Index2].Position;

            glm::vec3 FaceNormal = glm::cross(Position1 - Position0, Position2 - Position0);
            if (glm::dot(FaceNormal, FaceNormal) <= 0.0f)
            {
                continue;
            }

            FaceNormal = glm::normalize(FaceNormal);

            uint32_t TriangleIndices[3] = { Index0, Index1, Index2 };
            for (size_t Corner = 0; Corner < 3; Corner++)
            {
                uint32_t CurrentIndex = TriangleIndices[Corner];
                if (VertexImportedNormals[CurrentIndex] != 0u)
                {
                    continue;
                }

                if (VertexUseCount[CurrentIndex] > 1u)
                {
                    SVertex SplitVertex = NewVertices[CurrentIndex];
                    SplitVertex.Normal = FaceNormal;

                    const uint32_t SplitIndex = static_cast<uint32_t>(NewVertices.size());
                    NewVertices.push_back(SplitVertex);
                    VertexImportedNormals.push_back(1u);

                    NewIndices[i + Corner] = SplitIndex;
                    VertexUseCount[CurrentIndex]--;
                }
                else
                {
                    NewVertices[CurrentIndex].Normal = FaceNormal;
                }
            }
        }
    }

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
        float f     = (std::abs(Denom) > 0.0f) ? (1.0f / Denom) : 0.0f;

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
        NewVertices[i].Tangent = Tangent;
    }

    LOG("... Finished calculating Tangents\n");

    assert(NewIndices.size() < UINT32_MAX);

    SBufferParams VertexBufferParams = { };
    VertexBufferParams.Size             = NewVertices.size() * sizeof(SVertex);
    VertexBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_RAY_TRACING_INPUT;
    VertexBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;

    pVertexBuffer = CBuffer::CreateWithData(pDevice, VertexBufferParams, nullptr, NewVertices.data());
    assert(pVertexBuffer != nullptr);
    VertexCount = NewVertices.size();

    SBufferParams IndexBufferParams = { };
    IndexBufferParams.Size             = NewIndices.size() * sizeof(uint32_t);
    IndexBufferParams.Usage            = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_RAY_TRACING_INPUT;
    IndexBufferParams.MemoryProperties = VK_GPU_BUFFER_USAGE;

    pIndexBuffer = CBuffer::CreateWithData(pDevice, IndexBufferParams, nullptr, NewIndices.data());
    assert(pIndexBuffer != nullptr);
    IndexCount = NewIndices.size();

    // Create a AccelerationStructure if RayTracing is supported
    if (pDevice->IsRayTracingSupported())
    {
        VkTransformMatrixKHR TransformMatrix =
        {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f
        };

        SAccelerationStructureBLASParams BLASParams;
        for (const SModel::SSubMesh& SubMesh : NewSubmeshes)
        {
            const bool bHasAlphaMask = (SubMesh.MaterialIndex >= 0 && static_cast<size_t>(SubMesh.MaterialIndex) < NewMaterials.size())
                ? (NewMaterials[SubMesh.MaterialIndex].AlphaMaskTex != nullptr)
                : false;

            SBLASGeometry& Geometry = BLASParams.Geometries.emplace_back();
            Geometry.Flags              = bHasAlphaMask ? 0 : VK_GEOMETRY_OPAQUE_BIT_KHR;
            Geometry.TransformMatrix    = TransformMatrix;
            Geometry.pVertexBuffer      = pVertexBuffer;
            Geometry.MaxVertexIndex     = VertexCount;
            Geometry.VertexBufferCount  = SubMesh.VertexCount;
            Geometry.VertexBufferOffset = SubMesh.VertexOffset;
            Geometry.VertexStride       = sizeof(SVertex);
            Geometry.pIndexBuffer       = pIndexBuffer;
            Geometry.IndexBufferOffset  = SubMesh.IndexOffset;
            Geometry.IndexBufferCount   = SubMesh.IndexCount;
        }

        pAccelerationStructure = CAccelerationStructure::CreateBLAS(pDevice, BLASParams);
        assert(pAccelerationStructure != nullptr);
    }

    Indicies  = std::move(NewIndices);
    Vertices  = std::move(NewVertices);
    SubMeshes = std::move(NewSubmeshes);
    Materials = std::move(NewMaterials);
    return true;
}
