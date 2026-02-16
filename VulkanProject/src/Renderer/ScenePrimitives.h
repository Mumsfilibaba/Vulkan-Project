#pragma once
#include "Core.h"

enum class EObjectType : uint32_t
{
    RootNode = 0,
    Sphere   = 1,
    Quad     = 2,
    Mesh     = 3
};

struct SSphereHLSL
{
    // 0-16
    glm::vec3 Position;
    float     Radius;
    // 16-20
    uint32_t MaterialIndex;

    // Padding
    uint32_t Padding0;
    uint32_t Padding1;
    uint32_t Padding2;
};

struct SQuadHLSL
{
    // 0-16
    glm::vec4 Position;
    // 16-32
    glm::vec4 Edge0;
    // 32-48
    glm::vec4 Edge1;
    // 48-52
    uint32_t MaterialIndex;

    // Padding
    uint32_t Padding0;
    uint32_t Padding1;
    uint32_t Padding2;
};

struct SMeshHLSL
{
    // 0-16
    uint32_t BoundingBoxIndex = 0;

    // Padding for 16-byte alignment before matrix data.
    uint32_t Padding0 = 0;
    uint32_t Padding1 = 0;
    uint32_t Padding2 = 0;

    // 16-80
    glm::mat4 LocalToWorld = glm::identity<glm::mat4>();
    // 80-144
    glm::mat4 WorldToLocal = glm::identity<glm::mat4>();
};

struct SMaterialHLSL
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
    float SpecularChance;
    float SpecularRoughness;
    float IncidenceOfRefraction;
    float RefractionChance;
    // 80-84
    float RefractionRoughness;
    // 84-92
    uint32_t AlbedoTexIndex;
    uint32_t NormalTexIndex;
    uint32_t AlphaMaskTexIndex;
    // 92-100
    uint32_t MetallicTexIndex;
    uint32_t RoughnessTexIndex;

    // Padding
    uint32_t Padding0;
    uint32_t Padding1;
};

struct STriangleInfoHLSL
{
    // 0-4
    int32_t MaterialIndex;
};