#include "Model.h"

#include <tiny_obj_loader.h>

Model::~Model()
{
	delete m_pVertexBuffer;
	delete m_pIndexBuffer;
}


bool Model::LoadFromFile(const std::string& filepath, VulkanContext* pContext, VulkanDeviceAllocator* pAllocator)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str()))
	{
		std::cout << "Failed to load model '" << filepath << "'" << std::endl;
		if (!warn.empty())
			std::cout << "  Warning: " << warn << std::endl;
		if (!err.empty())
			std::cout << "  Error: " << err << std::endl;
		
		return false;
	}
	else
	{
		std::cout << "Loaded model '" << filepath << "'" << std::endl;
		if (!warn.empty())
			std::cout << "  Warning: " << warn << std::endl;
	}
	
	std::vector<Vertex> 	vertices;
	std::vector<uint16_t> 	indices;
	std::unordered_map<Vertex, uint16_t, VertexHasher> uniqueVertices = {};
	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};
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
	
	BufferParams vertexBufferParams = {};
	vertexBufferParams.SizeInBytes 		= vertices.size() * sizeof(Vertex);
	vertexBufferParams.Usage 			= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vertexBufferParams.MemoryProperties = VK_CPU_BUFFER_USAGE;
	m_pVertexBuffer = Buffer::Create(pContext, vertexBufferParams, pAllocator);

	void* pCPUMem = m_pVertexBuffer->Map();
	memcpy(pCPUMem, vertices.data(), vertexBufferParams.SizeInBytes);
	m_pVertexBuffer->Unmap();

	BufferParams indexBufferParams = {};
	indexBufferParams.SizeInBytes 		= indices.size() * sizeof(uint16_t);
	indexBufferParams.Usage 			= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	indexBufferParams.MemoryProperties 	= VK_CPU_BUFFER_USAGE;
	m_pIndexBuffer = Buffer::Create(pContext, indexBufferParams, pAllocator);

	pCPUMem = m_pIndexBuffer->Map();
	memcpy(pCPUMem, indices.data(), indexBufferParams.SizeInBytes);
	m_pIndexBuffer->Unmap();
	
	m_VertexCount = vertices.size();
	m_IndexCount = indices.size();
	return true;
}
