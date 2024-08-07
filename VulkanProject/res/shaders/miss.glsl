#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

#include "hw_ray_trace_common.glsl"

#define BACKGROUND_TYPE_NONE 0
#define BACKGROUND_TYPE_GRADIENT 1
#define BACKGROUND_TYPE_SKYBOX 2

layout(location = 0) rayPayloadInEXT FRayPayLoad RayPayLoad;

layout(binding = 5) uniform SceneBufferObject
{
    FSceneSettings Settings;
} uScene;

layout (set = 1, binding = 0) uniform samplerCube uCubeTextures[];

void main()
{
    if (uScene.Settings.BackgroundType == BACKGROUND_TYPE_NONE)
    {
        RayPayLoad.HitEmissive = vec3(0.0, 0.0, 0.0);
    }
    else if (uScene.Settings.BackgroundType == BACKGROUND_TYPE_GRADIENT)
    {
        const vec3  RayDir = normalize(gl_WorldRayDirectionEXT);
        const float Alpha  = 0.5 * (RayDir.y + 1.0);
        const vec3  Color  = (1.0 - Alpha) * vec3(1.0, 1.0, 1.0) + Alpha * vec3(0.5, 0.7, 1.0);
        
        const float Strength = max(1.0, uScene.Settings.GradientLightStrength);
        RayPayLoad.HitEmissive = Color * Strength;
    }
    else if (uScene.Settings.BackgroundType == BACKGROUND_TYPE_SKYBOX)
    {
        const vec3 RayDir      = normalize(gl_WorldRayDirectionEXT);
        const vec4 SkyboxColor = texture(uCubeTextures[0], RayDir);
        RayPayLoad.HitEmissive = SkyboxColor.rgb;
    }
}