#pragma once
#include "Core.h"

namespace Math
{
    template<typename T>
    inline T AlignUp(T Value, size_t Alignment)
    {
        const size_t Mask = Alignment - 1;
        return (T)(((size_t)Value + Mask) & ~Mask);
    }

    template<typename T>
    inline T AlignDown(T Value, size_t Alignment)
    {
        const size_t Mask = Alignment - 1;
        return (T)((size_t)Value & ~Mask);
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
