#pragma once
#include "DeviceChild.h"
#include <vulkan/vulkan.h>

struct FQueryParams
{
    VkQueryType QueryType;
    uint32_t    QueryCount;
};

class FQuery : public FDeviceChild
{
public:
    static FQuery* Create(FDevice* pDevice, const FQueryParams& Params);
    
    FQuery(FDevice* pDevice);
    ~FQuery();

    void Reset(uint32_t FirstQuery = 0, uint32_t QueryCount = 0);
    bool GetData(uint32_t FirstQuery, uint32_t QueryCount, uint64_t DataSize, void* pData, VkDeviceSize Stride, VkQueryResultFlags Flags);
    
    VkQueryPool GetQueryPool() const
    {
        return m_QueryPool;
    }

private:
    VkQueryPool m_QueryPool;
    VkQueryType m_QueryType;
    uint32_t    m_NumQueries;
};
