#ifndef RANDOM_HLSLI
#define RANDOM_HLSLI

#include "math.hlsli"

uint InitRandom(uint2 Pixel, uint Width, uint FrameIndex)
{
    const uint BackOff = 16u;

    uint v0 = Pixel.x + Pixel.y * Width;
    uint v1 = FrameIndex;
    uint s0 = 0u;

    [loop]
    for (uint n = 0u; n < BackOff; n++)
    {
        s0 += 0x9e3779b9u;
        v0 += ((v1 << 4) + 0xa341316cu) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4u);
        v1 += ((v0 << 4) + 0xad90777du) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761eu);
    }

    return v0;
}

uint XORShift(uint Value)
{
    Value ^= Value << 13;
    Value ^= Value >> 17;
    Value ^= Value << 5;
    return Value;
}

uint WangHash(inout uint Seed)
{
    Seed = uint(Seed ^ 61u) ^ uint(Seed >> 16u);
    Seed *= 9u;
    Seed = Seed ^ (Seed >> 4u);
    Seed *= 0x27d4eb2du;
    Seed = Seed ^ (Seed >> 15u);
    return Seed;
}

int NextRandomInt(inout uint Seed)
{
    Seed = WangHash(Seed);
    return int(Seed);
}

float NextRandom(inout uint Seed)
{
    Seed = uint(NextRandomInt(Seed));
    return float(Seed) / 4294967296.0;
}

float NextRandom(inout uint Seed, float MinValue, float MaxValue)
{
    return MinValue + (MaxValue - MinValue) * NextRandom(Seed);
}

float3 NextRandomVec3(inout uint Seed)
{
    return float3(NextRandom(Seed), NextRandom(Seed), NextRandom(Seed));
}

float3 NextRandomVec3(inout uint Seed, float MinValue, float MaxValue)
{
    return float3(
        NextRandom(Seed, MinValue, MaxValue),
        NextRandom(Seed, MinValue, MaxValue),
        NextRandom(Seed, MinValue, MaxValue));
}

float3 NextRandomUnitSphereVec3(inout uint Seed)
{
    const float z = NextRandom(Seed) * 2.0 - 1.0;
    const float a = NextRandom(Seed) * TWO_PI;
    const float r = sqrt(1.0 - z * z);
    const float x = r * cos(a);
    const float y = r * sin(a);
    return float3(x, y, z);
}

float3 NextRandomHemisphere(inout uint Seed, float3 Normal)
{
    const float3 UnitVector = NextRandomUnitSphereVec3(Seed);
    return dot(UnitVector, Normal) <= 0.0 ? -UnitVector : UnitVector;
}

#endif


