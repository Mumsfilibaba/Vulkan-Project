#pragma once
#include "Core.h"

struct FSphere
{
    // 0-16
    glm::vec3 Position;
    float     Radius;
    // 16-20
    uint32_t  MaterialIndex;

    // Padding
    uint32_t  Padding0;
    uint32_t  Padding1;
    uint32_t  Padding2;
};

struct FPlane
{
    // 0-16
    glm::vec3 Normal;
    float     Distance;
    // 16-20
    uint32_t  MaterialIndex;

    // Padding
    uint32_t  Padding0;
    uint32_t  Padding1;
    uint32_t  Padding2;
};

struct FQuad
{
    // 0-16
    glm::vec4 Position;
    // 16-32
    glm::vec4 Edge0;
    // 32-48
    glm::vec4 Edge1;
    // 48-52
    uint32_t  MaterialIndex;

    // Padding
    uint32_t  Padding0;
    uint32_t  Padding1;
    uint32_t  Padding2;
};

struct FTriangle
{
    // 0-12
    uint32_t Index0;
    uint32_t Index1;
    uint32_t Index2;

    // Padding
    uint32_t Padding0;
};

struct FTriangleMesh
{
    // 0-32
    glm::vec4 BoxMin;
    glm::vec4 BoxMax;
    // 32-44
    uint32_t  StartTriangle;
    uint32_t  NumTriangles;
    uint32_t  MaterialIndex;

    // Padding
    uint32_t  Padding0;
};

struct FMaterial
{
    // 0-16
    glm::vec4 AlbedoColor;
    // 16-32
    glm::vec4 EmissiveColor;
    // 32-48
    glm::vec4 SpecularColor;
    // 48-64
    glm::vec4 AbsorbtionColor;
    // 64-80
    float     SpecularChance;
    float     SpecularRoughness;
    float     IncidenceOfRefraction;
    float     RefractionChance;
    // 80-84
    float     RefractionRoughness;

    // Padding
    uint32_t  Padding0;
    uint32_t  Padding1;
    uint32_t  Padding2;
};