#ifndef TONEMAP_H
#define TONEMAP_H

vec3 RTTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786f) - 0.000090537f;
    vec3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

vec3 AcesFitted(vec3 Color)
{
    const mat3 InputMatrix =
    {
        { 0.59719f, 0.35458f, 0.04823f },
        { 0.07600f, 0.90834f, 0.01566f },
        { 0.02840f, 0.13383f, 0.83777f },
    };

    const mat3 OutputMatrix =
    {
        { 1.60475f, -0.53108f, -0.07367f },
        { -0.10208f, 1.10813f, -0.00605f },
        { -0.00327f, -0.07276f, 1.07602f },
    };

    Color = InputMatrix * Color;
    Color = RTTAndODTFit(Color);
    return clamp(OutputMatrix * Color, 0.0f, 1.0f);
}

vec3 ReinhardSimple(vec3 Color, float Intensity)
{
    return Color / (vec3(Intensity) + Color);
}

#endif