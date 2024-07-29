#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) rayPayloadInEXT vec3 HitValue;

void main()
{
    HitValue = vec3(0.0, 0.0, 0.2);
}