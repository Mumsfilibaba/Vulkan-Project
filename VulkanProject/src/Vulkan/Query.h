#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>

struct QueryParams
{
    VkQueryType queryType;
    uint32_t    queryCount;
};

class Query
{
public:
    static Query* Create(class VulkanContext* pContext, const QueryParams& params);
    
    Query(VkDevice device);
    ~Query();

    void Reset(uint32_t firstQuery = 0, uint32_t queryCount = 0);
    
    bool GetData(uint32_t firstQuery, uint32_t queryCount, uint64_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags);
    
    VkQueryPool GetQueryPool() const
    {
        return m_QueryPool;
    }

private:
    VkDevice    m_Device;
    VkQueryPool m_QueryPool;
    VkQueryType m_QueryType;
    uint32_t    m_NumQueries;
};
