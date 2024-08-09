#version 450
#extension GL_ARB_separate_shader_objects : require

layout(location = 0) out vec4 outColor;

layout(push_constant, std430) uniform PushConstant
{
    vec4 Color;
} Constants;

void main() 
{
    outColor = vec4(Constants.Color.rgb, 1.0);
}