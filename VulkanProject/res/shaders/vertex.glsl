#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;

layout(binding = 0) uniform CameraBufferObject 
{
    // 0-64
    mat4 Projection;
    // 64-128
    mat4 View;
    // 128-160
    vec4 Position;
    vec4 Forward;
    // 160-164
    float FieldOfViewDegrees;

    // Padding
    uint Padding0;
    uint Padding1;
    uint Padding2;
} uCamera;

void main() 
{
    gl_Position = uCamera.Projection * uCamera.View * vec4(inPosition, 1.0);
}
