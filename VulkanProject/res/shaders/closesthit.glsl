#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) rayPayloadInEXT vec3 HitValue;
hitAttributeEXT vec2 Attribs;

void main()
{
  const vec3 BarycentricCoords = vec3(1.0f - Attribs.x - Attribs.y, Attribs.x, Attribs.y);
  HitValue = BarycentricCoords;
}