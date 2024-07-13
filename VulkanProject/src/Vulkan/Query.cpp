#include "Query.h"
#include "Device.h"
#include "Extensions.h"

FQuery* FQuery::Create(class FDevice* pDevice, const FQueryParams& Params)
{
    FQuery* pQuery = new FQuery(pDevice);
    
    VkQueryPoolCreateInfo QueryCreateInfo;
    ZERO_STRUCT(&QueryCreateInfo);
    
    QueryCreateInfo.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    QueryCreateInfo.queryType  = pQuery->m_QueryType  = Params.QueryType;
    QueryCreateInfo.queryCount = pQuery->m_NumQueries = Params.QueryCount;
    
    VkResult Result = vkCreateQueryPool(pDevice->GetDevice(), &QueryCreateInfo, nullptr, &pQuery->m_QueryPool);
    if (Result != VK_SUCCESS)
    {
        std::cout << "vkCreateQueryPool failed. Error: " << Result << '\n';
        return nullptr;
    }
    else
    {
        std::cout << "Created query\n";
    }
    
    return pQuery;
}
    
FQuery::FQuery(FDevice* pDevice)
    : FDeviceChild(pDevice)
    , m_QueryPool(VK_NULL_HANDLE)
{
}

FQuery::~FQuery()
{
    if (m_QueryPool != VK_NULL_HANDLE)
    {
        vkDestroyQueryPool(GetDevice()->GetDevice(), m_QueryPool, nullptr);
        m_QueryPool = VK_NULL_HANDLE;
    }
}

void FQuery::Reset(uint32_t FirstQuery, uint32_t QueryCount)
{
    if (!QueryCount)
    {
        QueryCount = m_NumQueries;
    }
    
    vkResetQueryPool(GetDevice()->GetDevice(), m_QueryPool, FirstQuery, QueryCount);
}

bool FQuery::GetData(uint32_t FirstQuery, uint32_t QueryCount, uint64_t DataSize, void* pData, VkDeviceSize Stride, VkQueryResultFlags Flags)
{
    VkResult Result = vkGetQueryPoolResults(GetDevice()->GetDevice(), m_QueryPool, FirstQuery, QueryCount, DataSize, pData, Stride, Flags);
    if (Result != VK_SUCCESS)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void FQuery::SetDebugName(const char* DebugName)
{
    if (FExtensions::vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT DebugNameInfo;
        ZERO_STRUCT(&DebugNameInfo);
        
        DebugNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        DebugNameInfo.objectType   = VK_OBJECT_TYPE_QUERY_POOL;
        DebugNameInfo.pObjectName  = DebugName;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_QueryPool);

        VkResult Result = FExtensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            std::cout << "Failed to set name '" << DebugNameInfo.pObjectName << "'.Error: " << Result << std::endl;
        }
    }
}