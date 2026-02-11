#pragma once
#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(disable : 4201)
#endif

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <array>
#include <string>
#include <algorithm>
#include <set>
#include <chrono>
#include <future>
#include <atomic>
#include <fstream>
#include <iostream>

// GLM stuff
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>

// Vulkan
#include <vulkan/vulkan.h>
#if PLATFORM_MAC
    #include <MoltenVK/vk_mvk_moltenvk.h>
#endif

// GLFW
#ifndef EXLUDE_GLFW
    #define GLFW_INCLUDE_VULKAN
    #include <GLFW/glfw3.h>
#endif

// Helper Defines
#define ZERO_MEMORY(dst, size) memset(dst, 0, size)
#define ZERO_STRUCT(dst) memset(dst, 0, sizeof(std::remove_pointer_t<decltype(dst)>))

#if PLATFORM_WINDOWS
    bool HACK_IsDebuggerPresent();

    #define DEBUG_BREAK() \
        do \
        { \
            if (HACK_IsDebuggerPresent()) \
            { \
                __debugbreak(); \
            } \
        } while(false)
#elif PLATFORM_MAC
    #define DEBUG_BREAK __builtin_trap
#endif

#define SAFE_DELETE(pObject) \
    do \
    { \
        if (pObject) \
        { \
            delete (pObject); \
            pObject = nullptr; \
        } \
    } while(false)

#define LOG(Message, ...) \
    do \
    { \
        printf(Message, ##__VA_ARGS__); \
    } while(false)
