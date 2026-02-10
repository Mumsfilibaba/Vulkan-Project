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

    static bool IsMouseButtonDown(int32_t button)
    {
        return glfwGetMouseButton(CApplication::Get().GetWindow(), button) == GLFW_PRESS;
    }

    static bool IsMouseButtonUp(int32_t button)
    {
        return glfwGetMouseButton(CApplication::Get().GetWindow(), button) == GLFW_RELEASE;
    }

    static glm::vec2 GetMousePosition()
    {
        double MouseX = 0.0;
        double MouseY = 0.0;
        glfwGetCursorPos(CApplication::Get().GetWindow(), &MouseX, &MouseY);
        return glm::vec2(static_cast<float>(MouseX), static_cast<float>(MouseY));
    }
};
