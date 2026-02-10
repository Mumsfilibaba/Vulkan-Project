#pragma once
#include "Application.h"

struct Input
{
    static bool IsKeyDown(int32_t key)
    {
        return glfwGetKey(CApplication::Get().GetWindow(), key) == GLFW_PRESS;
    }

    static bool IsKeyUp(int32_t key)
    {
        return glfwGetKey(CApplication::Get().GetWindow(), key) == GLFW_RELEASE;
    }
};
