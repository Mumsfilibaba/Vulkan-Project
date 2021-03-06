#pragma once
#include <iostream>
#include <cassert>
#include <cstdint>

#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <set>

// GLM stuff
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

// Vulkan
#include <vulkan/vulkan.h>

// GLFW
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
