#pragma once
#include "Core.h"
#include "VulkanDeviceAllocator.h"

#define VK_CPU_BUFFER_USAGE (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
#define VK_GPU_BUFFER_USAGE (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)

struct BufferParams
{
	VkMemoryPropertyFlags MemoryProperties = 0;
	uint32_t Usage = 0;
	VkDeviceSize SizeInBytes = 0;
};

class VulkanBuffer
{
public:
	inline VulkanBuffer(VkDevice device, VkPhysicalDevice physicalDevice, const BufferParams& params, VulkanDeviceAllocator* pAllocator)
		: m_Device(device),
		m_PhysicalDevice(physicalDevice),
		m_pAllocator(pAllocator),
		m_Buffer(VK_NULL_HANDLE),
		m_DeviceMemory(VK_NULL_HANDLE),
		m_SizeInBytes(0),
		m_Allocation()
	{
		Init(params);
	}

	inline ~VulkanBuffer()
	{
		if (m_Buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(m_Device, m_Buffer, nullptr);
			m_Buffer = VK_NULL_HANDLE;
		}

		if (m_pAllocator)
		{
			m_pAllocator->Deallocate(m_Allocation);
		}
		else
		{
			if (m_DeviceMemory != VK_NULL_HANDLE)
			{
				vkFreeMemory(m_Device, m_DeviceMemory, nullptr);
				m_DeviceMemory = VK_NULL_HANDLE;
			}
		}
	}
	
	inline void* Map()
	{
		void* pResult = nullptr;
		if (m_pAllocator)
		{
			pResult = (void*)m_Allocation.pHostMemory;
		}
		else
		{
			VkResult result = vkMapMemory(m_Device, m_DeviceMemory, 0, m_SizeInBytes, 0, &pResult);
			if (result != VK_SUCCESS)
			{
				std::cout << "vkMapMemory failed. Error: " << result << std::endl;
			}
		}
		
		return pResult;
	}

	inline void Unmap()
	{
		if (!m_pAllocator)
		{
			vkUnmapMemory(m_Device, m_DeviceMemory);
		}
	}

	
	inline VkBuffer GetBuffer() const
	{
		return m_Buffer;
	}
	
private:
	void Init(const BufferParams& params);
	
private:
	VulkanDeviceAllocator* m_pAllocator;
	VkDevice m_Device;
	VkPhysicalDevice m_PhysicalDevice;
	VkBuffer m_Buffer;
	VkDeviceMemory m_DeviceMemory;
	VkDeviceSize m_SizeInBytes;
	VkDeviceAllocation m_Allocation;
};
