#ifndef MATH_H
#define MATH_H

#define PI (3.14159265358979);

vec3 RealReflect(vec3 v, vec3 n) 
{
    return v - 2.0 * dot(v, n) * n;
}

float LengthSquared(vec3 v)
{
    return v.x*v.x + v.y*v.y + v.z*v.z;
}

vec3 RealRefract(vec3 uv, vec3 n, float EtaiOverEtat) 
{
    const float CosTheta = min(dot(-uv, n), 1.0);
    vec3 rOutPerp     = EtaiOverEtat * (uv + CosTheta * n);
    vec3 rOutParallel = -sqrt(abs(1.0 - dot(rOutPerp, rOutPerp))) * n;
    return rOutPerp + rOutParallel;
}

#endif
