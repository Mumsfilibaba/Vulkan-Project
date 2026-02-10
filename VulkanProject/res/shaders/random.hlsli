#ifndef RANDOM_HLSLI
#define RANDOM_HLSLI

#include "math.hlsli"

uint InitRandom(uint2 pixel, uint width, uint frameIndex)
{
    const uint backOff = 16u;

    uint v0 = pixel.x + pixel.y * width;
    uint v1 = frameIndex;
    uint s0 = 0u;

    [loop]
    for (uint n = 0u; n < backOff; n++)
    {
        s0 += 0x9e3779b9u;
        v0 += ((v1 << 4) + 0xa341316cu) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4u);
        v1 += ((v0 << 4) + 0xad90777du) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761eu);
    }

    return v0;
}

uint XORShift(uint value)
{
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    return value;
}

uint WangHash(inout uint seed)
{
    seed = uint(seed ^ 61u) ^ uint(seed >> 16u);
    seed *= 9u;
    seed = seed ^ (seed >> 4u);
    seed *= 0x27d4eb2du;
    seed = seed ^ (seed >> 15u);
    return seed;
}

int NextRandomInt(inout uint seed)
{
    seed = WangHash(seed);
    return int(seed);
}

float NextRandom(inout uint seed)
{
    seed = uint(NextRandomInt(seed));
    return float(seed) / 4294967296.0;
}

float NextRandom(inout uint seed, float minValue, float maxValue)
{
    return minValue + (maxValue - minValue) * NextRandom(seed);
}

float3 NextRandomVec3(inout uint seed)
{
    return float3(NextRandom(seed), NextRandom(seed), NextRandom(seed));
}

float3 NextRandomVec3(inout uint seed, float minValue, float maxValue)
{
    return float3(
        NextRandom(seed, minValue, maxValue),
        NextRandom(seed, minValue, maxValue),
        NextRandom(seed, minValue, maxValue));
}

float3 NextRandomUnitSphereVec3(inout uint seed)
{
    const float z = NextRandom(seed) * 2.0 - 1.0;
    const float a = NextRandom(seed) * TWO_PI;
    const float r = sqrt(1.0 - z * z);
    const float x = r * cos(a);
    const float y = r * sin(a);
    return float3(x, y, z);
}

float3 NextRandomHemisphere(inout uint seed, float3 normal)
{
    const float3 unitVector = NextRandomUnitSphereVec3(seed);
    return dot(unitVector, normal) <= 0.0 ? -unitVector : unitVector;
}

#endif
