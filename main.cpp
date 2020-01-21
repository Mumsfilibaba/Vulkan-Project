#include "Application.h"
#include <iostream>

int main()
{
    Application* pApp = Application::Create();
    pApp->Init();

    while (pApp->IsRunning())
        pApp->Run();
    
    pApp->Release();
    return 0;
}