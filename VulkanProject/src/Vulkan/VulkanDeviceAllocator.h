#pragma once
#include "Core.h"

#include <vulkan/vulkan.h>
#include <vector>

struct VkMemoryBlock;
struct VkDeviceAllocation;
class VkMemoryPage;

struct VkMemoryBlock
{
	VkMemoryPage*	pPage = nullptr;
	VkMemoryBlock*	pNext = nullptr;
	VkMemoryBlock*	pPrevious = nullptr;
	VkDeviceSize    SizeInBytes = 0;
	VkDeviceSize    PaddedSizeInBytes = 0;
	VkDeviceSize    DeviceMemoryOffset = 0;
	bool   IsFree = true;
	uint32 ID = 0;
};

struct VkDeviceAllocation
{
	VkMemoryBlock* pBlock = nullptr;
	uint8* pHostMemory = nullptr;
	VkDeviceSize    SizeInBytes = 0;
	VkDeviceSize    DeviceMemoryOffset = 0;
	VkDeviceMemory  DeviceMemory = VK_NULL_HANDLE;
};

class VkMemoryPage
{
public:
	DECL_NO_COPY(VkMemoryPage);

	VkMemoryPage(VkDevice device, VkPhysicalDevice physicalDevice, uint32 id, VkDeviceSize sizeInBytes, uint32 memoryType, VkMemoryPropertyFlags properties);
	~VkMemoryPage();

	bool Allocate(VkDeviceAllocation& allocation, VkDeviceSize sizeInBytes, VkDeviceSize alignment, VkDeviceSize granularity);
	void Deallocate(VkDeviceAllocation& allocation);

	inline bool IsEmpty() const { return m_pHead->pPrevious == nullptr && m_pHead->pNext == nullptr && m_pHead->IsFree; }
	inline uint64 GetSizeInBytes() const { return m_SizeInBytes; }
	inline uint32 GetMemoryType() const { return m_MemoryType; }
private:
	bool IsOnSamePage(VkDeviceSize aOffset, VkDeviceSize aSize, VkDeviceSize bOffset, VkDeviceSize pageSize);
	void Init();
	void Map();
	void Unmap();
private:
	VkDevice m_Device;
	VkPhysicalDevice m_PhysicalDevice;
	VkDeviceMemory	m_DeviceMemory;
	VkMemoryBlock* m_pHead;
	uint8* m_pHostMemory;
	VkMemoryPropertyFlags m_Properties;
	const uint32 m_ID;
	const uint32 m_MemoryType;
	const uint64 m_SizeInBytes;
	uint32 m_BlockCount;
	bool m_IsMapped;
};

class VulkanDeviceAllocator
{
public:
	DECL_NO_COPY(VulkanDeviceAllocator);

	VulkanDeviceAllocator(VkDevice device, VkPhysicalDevice physicalDevice);
	~VulkanDeviceAllocator();

	bool Allocate(VkDeviceAllocation& allocation, const VkMemoryRequirements& memoryRequirements, VkMemoryPropertyFlags properties);
	void Deallocate(VkDeviceAllocation& allocation);
	void EmptyGarbageMemory();

	inline uint64 GetTotalReserved() const { return m_TotalReserved; }
	inline uint64 GetTotalAllocated() const { return m_TotalAllocated; }
private:
	VkDevice m_Device;
	VkPhysicalDevice m_PhysicalDevice;
	VkDeviceSize m_BufferImageGranularity;
	std::vector<VkMemoryPage*> m_Pages;
	std::vector<std::vector<VkDeviceAllocation>> m_GarbageMemory;
	uint64 m_FrameIndex;
	uint64 m_TotalAllocated;
	uint64 m_TotalReserved;
	uint64 m_MaxAllocations;
};

