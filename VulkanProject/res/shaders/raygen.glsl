#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(binding = 0, set = 0)          uniform accelerationStructureEXT uAccelerationStructure;
layout(binding = 1, set = 0, rgba32f) uniform image2D uOutput;

layout(binding = 2) uniform CameraBufferObject 
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

layout(location = 0) rayPayloadEXT vec3 HitValue;

void main() 
{
	vec2 PixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
	vec2 TexCoord    = PixelCenter / vec2(gl_LaunchSizeEXT.xy);
    TexCoord.y = 1.0 - TexCoord.y;

	vec2 d = TexCoord * 2.0 - 1.0;

	vec4 Origin    = uCamera.InverseView       * vec4(0.0, 0.0, 0.0, 1.0);
	vec4 Target    = uCamera.InverseProjection * vec4(d.x, d.y, 1.0, 1.0);
	vec4 Direction = uCamera.InverseView       * vec4(normalize(Target.xyz), 0.0);

	float MinT = 0.001;
	float MaxT = 10000.0;

    HitValue = vec3(0.0);

    traceRayEXT(uAccelerationStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, Origin.xyz, MinT, Direction.xyz, MaxT, 0);

	imageStore(uOutput, ivec2(gl_LaunchIDEXT.xy), vec4(HitValue, 0.0));
}
