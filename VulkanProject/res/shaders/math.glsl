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

float Reflectance(float cosine, float RefractionIndex) 
{
    // Use Schlick's approximation for reflectance.
    float r0 = (1.0 - RefractionIndex) / (1.0 + RefractionIndex);
    r0 = r0 * r0;
    return r0 + (1.0 - r0) * pow((1.0 - cosine), 5.0);
}

#endif
