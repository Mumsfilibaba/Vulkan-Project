#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inFragCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D sceneTexture;

// ACES tone mapping curve fit to go from HDR to LDR
//https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESFilm(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 LinearToSRGB(vec3 LinearColor)
{
    vec3 SRGBColor;
    for (int i = 0; i < 3; ++i)
    {
        if (LinearColor[i] <= 0.0031308)
        {
            SRGBColor[i] = 12.92 * LinearColor[i];
        }
        else
        {
            SRGBColor[i] = 1.055 * pow(LinearColor[i], 1.0 / 2.4) - 0.055;
        }
    }

    return SRGBColor;
}

void main()
{
    vec3 Color = texture(sceneTexture, inFragCoord).rgb;
    Color = ACESFilm(Color);
    Color = LinearToSRGB(Color);
    outColor = vec4(Color, 1.0);
}
