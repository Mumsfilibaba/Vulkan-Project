#pragma once
#include "Core.h"
#include <type_traits>

namespace Math
{
    template<typename T>
    inline T AlignUp(T Value, size_t Alignment)
    {
        const size_t Mask = Alignment - 1;
        return static_cast<T>((static_cast<size_t>(Value) + Mask) & ~Mask);
    }

    template<typename T>
    inline T AlignDown(T Value, size_t Alignment)
    {
        const size_t Mask = Alignment - 1;
        return static_cast<T>(static_cast<size_t>(Value) & ~Mask);
    }

    inline float ToDegrees(float Radians)
    {
        return Radians * (180.0f / glm::pi<float>());
    }

    inline float ToRadians(float Degrees)
    {
        return Degrees * (glm::pi<float>() / 180.0f);
    }
}

namespace Hash
{
    template<typename T>
    inline size_t Value(const T& Input)
    {
        std::hash<T> Hasher;
        return Hasher(Input);
    }

    inline size_t Value(const glm::vec2& Input)
    {
        size_t Seed = std::hash<float>()(Input.x);
        Seed ^= std::hash<float>()(Input.y) + 0x9e3779b9 + (Seed << 6) + (Seed >> 2);
        return Seed;
    }

    inline size_t Value(const glm::vec3& Input)
    {
        size_t Seed = std::hash<float>()(Input.x);
        Seed ^= std::hash<float>()(Input.y) + 0x9e3779b9 + (Seed << 6) + (Seed >> 2);
        Seed ^= std::hash<float>()(Input.z) + 0x9e3779b9 + (Seed << 6) + (Seed >> 2);
        return Seed;
    }

    inline size_t Value(const glm::vec4& Input)
    {
        size_t Seed = std::hash<float>()(Input.x);
        Seed ^= std::hash<float>()(Input.y) + 0x9e3779b9 + (Seed << 6) + (Seed >> 2);
        Seed ^= std::hash<float>()(Input.z) + 0x9e3779b9 + (Seed << 6) + (Seed >> 2);
        Seed ^= std::hash<float>()(Input.w) + 0x9e3779b9 + (Seed << 6) + (Seed >> 2);
        return Seed;
    }

    template<typename T>
    inline void Combine(size_t& OutSeed, const T& Value)
    {
        OutSeed ^= Hash::Value(Value) + 0x9e3779b9 + (OutSeed << 6) + (OutSeed >> 2);
    }
}