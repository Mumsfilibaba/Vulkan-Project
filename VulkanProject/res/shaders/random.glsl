#ifndef RANDOM_GLSL
#define RANDOM_GLSL

#include "math.glsl"

// Source: https://github.com/NVIDIAGameWorks/GettingStartedWithRTXRayTracing/blob/master/11-OneShadowRayPerPixel/Data/Tutorial11/diffusePlus1ShadowUtils.hlsli
uint InitRandom(uvec2 Pixel, uint Width, uint FrameIndex)
{
	const uint BackOff = 16;
	
	uint v0 = Pixel.x + Pixel.y * Width;
	uint v1 = FrameIndex;
	uint s0 = 0;

	for (uint n = 0; n < BackOff; n++)
	{
		s0 += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
		v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
	}
	
	return v0;
}

// Xorshift*32
// Based on George Marsaglia's work: http://www.jstatsoft.org/v08/i14/paper
uint XORShift(uint Value)
{
	Value ^= Value << 13;
	Value ^= Value >> 17;
	Value ^= Value << 5;
	return Value;
}

uint WangHash(inout uint Seed)
{
    Seed = uint(Seed ^ uint(61)) ^ uint(Seed >> uint(16));
    Seed *= uint(9);
    Seed = Seed ^ (Seed >> 4);
    Seed *= uint(0x27d4eb2d);
    Seed = Seed ^ (Seed >> 15);
    return Seed;
}

int NextRandomInt(inout uint Seed)
{
	Seed = WangHash(Seed);
	return int(Seed);
}

float NextRandom(inout uint Seed)
{
	Seed = NextRandomInt(Seed);
	return float(Seed) / 4294967296.0;
}

float NextRandom(inout uint Seed, float Min, float Max)
{
	return Min + (Max - Min) * NextRandom(Seed);
}

vec3 NextRandomVec3(inout uint Seed)
{
	return vec3(NextRandom(Seed), NextRandom(Seed), NextRandom(Seed));
}

vec3 NextRandomVec3(inout uint Seed, float Min, float Max)
{
	return vec3(NextRandom(Seed, Min, Max), NextRandom(Seed, Min, Max), NextRandom(Seed, Min, Max));
}

vec3 NextRandomUnitSphereVec3(inout uint Seed)
{
	float z = NextRandom(Seed) * 2.0 - 1.0;
	float a = NextRandom(Seed) * TWO_PI;
	float r = sqrt(1.0 - z * z);
	float x = r * cos(a);
	float y = r * sin(a);
	return vec3(x, y, z);

#if 0
	vec3 Result;
	for (uint i = 0; i < 64; i++)
	{
		Result = NextRandomVec3(Seed, -1.0, 1.0);
		if (dot(Result, Result) < 1.0)
		{
			break;
		}
	}

	return normalize(Result);
#endif
}

vec3 NextRandomHemisphere(inout uint Seed, vec3 Normal)
{
	vec3 UnitVector = NextRandomUnitSphereVec3(Seed);

	float UdotN = dot(UnitVector, Normal);
	if (UdotN <= 0.0)
	{
		return -UnitVector;
	}
	else
	{
		return UnitVector;
	}
}

#endif
