#ifndef MATH_HLSLI
#define MATH_HLSLI

static const float PI           = 3.14159265358979;
static const float TWO_PI       = 6.28318530717958;
static const float LARGE_NUMBER = 1e30f;

float3 RealReflect(float3 v, float3 n)
{
    return v - 2.0 * dot(v, n) * n;
}

float LengthSquared(float3 v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

float3 RealRefract(float3 uv, float3 n, float etaiOverEtat)
{
    const float cosTheta = min(dot(-uv, n), 1.0);
    const float3 rOutPerp = etaiOverEtat * (uv + cosTheta * n);
    const float3 rOutParallel = -sqrt(abs(1.0 - dot(rOutPerp, rOutPerp))) * n;
    return rOutPerp + rOutParallel;
}

float Reflectance(float cosine, float refractionIndex)
{
    float r0 = (1.0 - refractionIndex) / (1.0 + refractionIndex);
    r0 = r0 * r0;
    return r0 + (1.0 - r0) * pow((1.0 - cosine), 5.0);
}

#endif
