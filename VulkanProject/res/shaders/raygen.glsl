#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

#include "hw_ray_trace_common.glsl"
#include "random.glsl"

#define RAY_OFFSET 0.001
#define MAX_DEPTH 1024

#define ENABLE_RUSSIAN_ROULETTE 1

layout(binding = 0) uniform accelerationStructureEXT uAccelerationStructure;

layout (binding = 1, rgba32f) uniform image2D uOutput;
layout (binding = 2, rgba32f) uniform image2D uPreviousFrame;

layout(binding = 3) uniform CameraBufferObject 
{
    // 0-64
    mat4 Projection;
    // 64-128
    mat4 View;
    // 128-192
    mat4 InverseProjection;
    // 192-256
    mat4 InverseView;
    // 256-288
    vec4 Position;
    vec4 Forward;
    // 288-292
    float FieldOfViewDegrees;

    // Padding
    uint Padding0;
    uint Padding1;
    uint Padding2;
} uCamera;

layout(binding = 4) uniform RandomBufferObject 
{
    // 0-8
    uint FrameIndex;
    uint HaltonIndex;

    // Padding
    uint Padding0;
    uint Padding1;
} uRandom;

layout(binding = 5) uniform SceneBufferObject 
{
    FSceneSettings Settings;
} uScene;

layout(location = 0) rayPayloadEXT FRayPayLoad RayPayLoad;

void main() 
{
    // Initialize a random Seed
    uint RandomSeed = InitRandom(uvec2(gl_LaunchIDEXT.xy), uint(gl_LaunchSizeEXT.x), uRandom.FrameIndex);

    vec2 Jitter      = vec2(NextRandom(RandomSeed), NextRandom(RandomSeed)) - 0.5;
	vec2 PixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);

    // Calculate TexCoord with Jitter
	vec2 TexCoord = (PixelCenter + Jitter) / vec2(gl_LaunchSizeEXT.xy);
    TexCoord.y = 1.0 - TexCoord.y;

	vec2 d = TexCoord * 2.0 - 1.0;

    // Calculate primary ray
	vec4 Origin    = uCamera.InverseView       * vec4(0.0, 0.0, 0.0, 1.0);
	vec4 Target    = uCamera.InverseProjection * vec4(d.x, d.y, 1.0, 1.0);
	vec4 Direction = uCamera.InverseView       * vec4(normalize(Target.xyz), 0.0);

	float MinT = 0.001;
	float MaxT = 10000.0;

    vec3 RayColor    = vec3(1.0);
    vec3 SampleColor = vec3(0.0);

    // Add one bounce (Primary ray)
    const uint NumBounces = min(uScene.Settings.NumBounces, MAX_DEPTH) + 1;
    for (uint i = 0; i < NumBounces; i++)
    {
        RayPayLoad.HitNormal   = vec3(0.0);
        RayPayLoad.HitPosition = vec3(0.0);
        RayPayLoad.HitAlbedo   = vec3(0.0);
        RayPayLoad.HitEmissive = vec3(0.0);

        // Trace-Ray
        traceRayEXT(uAccelerationStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, Origin.xyz, MinT, Direction.xyz, MaxT, 0);

        // Prepare next ray
        Direction = vec4(normalize(RayPayLoad.HitNormal + NextRandomUnitSphereVec3(RandomSeed)), 0.0);
        Origin    = vec4(RayPayLoad.HitPosition.xyz + (RayPayLoad.HitNormal * RAY_OFFSET), 0.0);

        // Add to the sample
        SampleColor += RayPayLoad.HitEmissive * RayColor;

        // Modify ray-color for next hit
        RayColor = RayPayLoad.HitAlbedo * RayColor;

    #if ENABLE_RUSSIAN_ROULETTE
        float Probability = max(RayColor.r, max(RayColor.g, RayColor.b));
        if (NextRandom(RandomSeed) > Probability)
        {
            break;
        }

        // Add the energy we 'lose' by randomly terminating paths
        RayColor /= Probability;
    #endif
    }
	
    // Accumulate samples over time
    vec4 PreviousColor = imageLoad(uPreviousFrame, ivec2(gl_LaunchIDEXT.xy));
    vec3 CurrentColor  = mix(PreviousColor.rgb, SampleColor, 1.0 / float(uRandom.FrameIndex + 1));
    imageStore(uOutput, ivec2(gl_LaunchIDEXT.xy), vec4(CurrentColor, 0.0));
}
