#pragma once
#include "Application.h"

struct FInput
{
    static bool IsKeyDown(int32_t key)
    {
        return glfwGetKey(FApplication::GetWindow(), key) == GLFW_PRESS;
    }

    static bool IsKeyUp(int32_t key)
    {
        return glfwGetKey(FApplication::GetWindow(), key) == GLFW_RELEASE;
    }
};