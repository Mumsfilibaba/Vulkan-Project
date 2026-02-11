#ifndef HALTON_HLSLI
#define HALTON_HLSLI

float RadicalInverse2(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

float RadicalInverse3(uint a)
{
    const float oneMinusEpsilon = 0.99999994f;

    const uint  base    = 3;
    const float invBase = 1.0f / float(base);

    uint  reversedDigits = 0;
    float invBaseN = 1.0f;

    while (a != 0)
    {
        const uint next  = a / base;
        const uint digit = a - next * base;

        reversedDigits = reversedDigits * base + digit;
        invBaseN *= invBase;
        
        a = next;
    }

    return min(reversedDigits * invBaseN, oneMinusEpsilon);
}

float2 Hammersley2(uint i, uint n)
{
    return float2(float(i) / float(n), RadicalInverse2(i));
}

float2 Halton23(uint i)
{
    return float2(RadicalInverse2(i), RadicalInverse3(i));
}

#endif
