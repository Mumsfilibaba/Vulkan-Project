#define EXLUDE_GLFW
#include "Core.h"

#if PLATFORM_WINDOWS
    #include <Windows.h>

    bool HACK_IsDebuggerPresent()
    {
        return IsDebuggerPresent();
    }
#endif

#undef EXLUDE_GLFW