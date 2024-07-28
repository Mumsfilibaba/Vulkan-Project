#pragma once
#include "Core.h"

class FDevice;

class FDeviceChild
{
public:
    FDeviceChild(FDevice* pDevice)
        : m_pDevice(pDevice)
    {
        assert(pDevice != nullptr);
    }

    ~FDeviceChild()
    {
        m_pDevice = nullptr;
    }
    
    FDevice* GetDevice() const
    {
        return m_pDevice;
    }

private:
    FDevice* m_pDevice;
};
