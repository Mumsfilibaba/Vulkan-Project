#ifndef MATH_H
#define MATH_H

#define PI (3.14159265358979f);

vec3 RealReflect(vec3 v, vec3 n) 
{
    return v - 2.0 * dot(v,n) * n;
}

#endif
