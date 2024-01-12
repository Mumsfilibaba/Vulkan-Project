#include "Query.h"
#include "Device.h"

FQuery* FQuery::Create(class FDevice* pDevice, const FQueryParams& params)
{
    FQuery* pQuery = new FQuery(pDevice->GetDevice());
    
    VkQueryPoolCreateInfo createInfo;
    ZERO_STRUCT(&createInfo);
    
    createInfo.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    createInfo.queryType  = pQuery->m_QueryType  = params.queryType;
    createInfo.queryCount = pQuery->m_NumQueries = params.queryCount;
    
    VkResult result = vkCreateQueryPool(pDevice->GetDevice(), &createInfo, nullptr, &pQuery->m_QueryPool);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkCreateQueryPool failed. Error: " << result << '\n';
        return nullptr;
    }
    else
    {
        std::cout << "Created query\n";
    }
    
    return pQuery;
}
    
FQuery::FQuery(VkDevice device)
    : m_Device(device)
    , m_QueryPool(VK_NULL_HANDLE)
{
}

FQuery::~FQuery()
{
    if (m_QueryPool != VK_NULL_HANDLE)
    {
        vkDestroyQueryPool(m_Device, m_QueryPool, nullptr);
        m_QueryPool = VK_NULL_HANDLE;
    }

    m_Device = VK_NULL_HANDLE;
}

void FQuery::Reset(uint32_t firstQuery, uint32_t queryCount)
{
    if (!queryCount)
    {
        queryCount = m_NumQueries;
    }
    
    vkResetQueryPool(m_Device, m_QueryPool, firstQuery, queryCount);
}

bool FQuery::GetData(uint32_t firstQuery, uint32_t queryCount, uint64_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags)
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
