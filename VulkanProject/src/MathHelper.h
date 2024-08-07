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
    inline void Combine(size_t& OutSeed, const T& Value)
    {
        std::hash<T> Hasher;
        OutSeed ^= Hasher(Value) + 0x9e3779b9 + (OutSeed << 6) + (OutSeed >> 2);
    }
}