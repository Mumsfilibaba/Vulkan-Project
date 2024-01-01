#include "Query.h"
#include "VulkanContext.h"

Query* Query::Create(class VulkanContext* pContext, const QueryParams& params)
{
    Query* newQuery = new Query(pContext->GetDevice());
    
    VkQueryPoolCreateInfo createInfo;
    ZERO_STRUCT(&createInfo);
    
    createInfo.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    createInfo.queryType  = newQuery->m_QueryType  = params.queryType;
    createInfo.queryCount = newQuery->m_NumQueries = params.queryCount;
    
    VkResult result = vkCreateQueryPool(pContext->GetDevice(), &createInfo, nullptr, &newQuery->m_QueryPool);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkCreateQueryPool failed. Error: " << result << std::endl;
        return nullptr;
    }
    else
    {
        std::cout << "Created query" << std::endl;
    }
    
    return newQuery;
}
    
Query::Query(VkDevice device)
    : m_Device(device)
    , m_QueryPool(VK_NULL_HANDLE)
{
}

Query::~Query()
{
    if (m_QueryPool != VK_NULL_HANDLE)
    {
        vkDestroyQueryPool(m_Device, m_QueryPool, nullptr);
        m_QueryPool = VK_NULL_HANDLE;
    }
}

void Query::Reset(uint32_t firstQuery, uint32_t queryCount)
{
    if (!queryCount)
    {
        queryCount = m_NumQueries;
    }
    
    vkResetQueryPool(m_Device, m_QueryPool, firstQuery, queryCount);
}

bool Query::GetData(uint32_t firstQuery, uint32_t queryCount, uint64_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags)
{
    VkResult result = vkGetQueryPoolResults(m_Device, m_QueryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
    if (result != VK_SUCCESS)
    {
        return false;
    }
    else
    {
        return true;
    }
}
