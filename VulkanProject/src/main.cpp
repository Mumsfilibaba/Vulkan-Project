#include "Application.h"
#include <iostream>

int main()
{
    CApplication* pApp = CApplication::Create();
    if (!pApp->Init())
    {
        pApp->Release();
        return 1;
    }
    
    StartApplicationLoop();

    while (IsApplicationRunning())
    {
        pApp->Tick();
    }
    
    pApp->Release();
    return 0;
}
