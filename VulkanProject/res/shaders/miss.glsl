#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

#include "hw_ray_trace_common.glsl"

layout(location = 0) rayPayloadInEXT FRayPayLoad RayPayLoad;

layout(binding = 5) uniform SceneBufferObject 
{
    FSceneSettings Settings;
} uScene;

void main()
{
    const vec3  RayDir = normalize(gl_WorldRayDirectionEXT);
    const float Alpha  = 0.5 * (RayDir.y + 1.0);
    const vec3  Color  = (1.0 - Alpha) * vec3(1.0, 1.0, 1.0) + Alpha * vec3(0.5, 0.7, 1.0);
    
    const float Strength = max(1.0, uScene.Settings.GradientLightStrength);
    RayPayLoad.HitEmissive = Color * Strength;
}