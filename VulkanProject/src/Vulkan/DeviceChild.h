#pragma once
#include "Core.h"

class CDevice;

class CDeviceChild
{
public:
    CDeviceChild(CDevice* pDevice)
        : m_pDevice(pDevice)
    {
        assert(pDevice != nullptr);
    }

    ~CDeviceChild()
    {
        m_pDevice = nullptr;
    }
    
    CDevice* GetDevice() const
    {
        return m_pDevice;
    }

private:
    CDevice* m_pDevice;
};
