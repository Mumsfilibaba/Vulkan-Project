#pragma once
#include "Vulkan/VulkanBuffer.h"
#include "Vulkan/VulkanContext.h"
#include "Vulkan/VulkanDeviceAllocator.h"

struct Vertex
{
	glm::vec3 Position;
	glm::vec2 TexCoord;
	glm::vec3 Color;
	
	bool operator==(const Vertex& other) const
	{
		return
			Position == other.Position &&
			TexCoord == other.TexCoord &&
			Color == other.Color;
	}

	static VkVertexInputBindingDescription GetBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding   = 0;
		bindingDescription.stride    = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static VkVertexInputAttributeDescription* GetAttributeDescriptions()
	{
		static VkVertexInputAttributeDescription attributeDescriptions[3];

		attributeDescriptions[0].binding    = 0;
		attributeDescriptions[0].location   = 0;
		attributeDescriptions[0].format     = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset     = offsetof(Vertex, Position);
		
		attributeDescriptions[1].binding    = 0;
		attributeDescriptions[1].location   = 1;
		attributeDescriptions[1].format     = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset     = offsetof(Vertex, TexCoord);

		attributeDescriptions[2].binding    = 0;
		attributeDescriptions[2].location   = 2;
		attributeDescriptions[2].format     = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset     = offsetof(Vertex, Color);

		return attributeDescriptions;
	}
};

struct VertexHasher
{
	size_t operator()(const Vertex& vertex) const
	{
		using namespace std;
		return ((hash<glm::vec3>()(vertex.Position) ^ (hash<glm::vec3>()(vertex.Color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.TexCoord) << 1);
	}
};

class Model
{
public:
	Model() = default;
	~Model();
	
	bool LoadFromFile(const std::string& filepath, VulkanContext* pContext, VulkanDeviceAllocator* pAllocator);
	
	inline VulkanBuffer* GetVertexBuffer() const
	{
		return m_pVertexBuffer;
	}
	
	inline VulkanBuffer* GetIndexBuffer() const
	{
		return m_pIndexBuffer;
	}
	
	inline uint32_t GetVertexCount() const
	{
		return m_VertexCount;
	}
	
	inline uint32_t GetIndexCount() const
	{
		return m_IndexCount;
	}
	
private:
	VulkanBuffer* m_pVertexBuffer = nullptr;
	VulkanBuffer* m_pIndexBuffer = nullptr;
	uint32_t m_VertexCount = 0;
	uint32_t m_IndexCount = 0;
};
