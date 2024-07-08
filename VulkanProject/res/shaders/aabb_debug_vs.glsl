#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;

layout(binding = 0) uniform CameraBufferObject 
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

layout(push_constant, std430) uniform PushConstant
{
    mat4 TransformMatrix;
    vec4 Color;
} Constants;

void main() 
{
    gl_Position = uCamera.Projection * uCamera.View * Constants.TransformMatrix * vec4(inPosition, 1.0);
}
