#pragma once
#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(disable : 4201)
#endif

#include <iostream>
#include <cassert>
#include <cstdint>

#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <set>
#include <chrono>

// GLM stuff
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/constants.hpp>

// Vulkan
#include <vulkan/vulkan.h>

// GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Helper Defines
#define ZERO_MEMORY(dst, size) memset(dst, 0, size)
#define ZERO_STRUCT(dst)       memset(dst, 0, sizeof(std::remove_pointer_t<decltype(dst)>))
