#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

#include "hw_ray_trace_common.glsl"
#include "random.glsl"
#include "primitives.glsl"

#define RAY_OFFSET 0.001
#define MAX_DEPTH 1024

#define VIEW_MODE_RENDER 0
#define VIEW_MODE_NORMALS 1
#define VIEW_MODE_ALBEDO 2
#define VIEW_MODE_BARYCENTRICS 3
#define VIEW_MODE_TEXCOORDS 4

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
    SSceneSettings Settings;
} uScene;

layout(binding = 7) buffer MaterialBuffer
{
    SMaterial Materials[];
};

layout (set = 1, binding = 0) uniform sampler2D uTextures[];

layout(location = 0) rayPayloadEXT SRayPayLoad RayPayLoad;

float FresnelReflectAmount(float N1, float N2, vec3 Normal, vec3 Incident, float F0, float F90)
{
    // Schlick aproximation
    float R0 = (N1 - N2) / (N1 + N2);
    R0 *= R0;

    float CosX = -dot(Normal, Incident);
    if (N1 > N2)
    {
        float N     = N1 / N2;
        float SinT2 = N * N * (1.0 - CosX * CosX);
        if (SinT2 > 1.0)
        {
            return F90;
        }

        CosX = sqrt(1.0 - SinT2);
    }

    float X  = 1.0 - CosX;
    float X2 = X * X;

    float Result = R0 + (1.0 - R0) * X2 * X2 * X;
    return mix(F0, F90, Result);
}

vec3 GetColorForRay(in vec3 Origin, in vec3 Direction, inout uint RandomSeed)
{
    // Start tracing rays
    vec3 RayColor    = vec3(1.0);
    vec3 SampleColor = vec3(0.0);

	float MinT = 0.001;
	float MaxT = 10000.0;

    // Add one bounce (Primary ray)
    const uint NumBounces = min(uScene.Settings.NumBounces, MAX_DEPTH);
    for (uint i = 0; i < NumBounces; i++)
    {
        RayPayLoad.HitNormal        = vec3(0.0);
        RayPayLoad.HitTangent       = vec3(0.0);
        RayPayLoad.HitPosition      = vec3(0.0);
        RayPayLoad.HitBarycentrics  = vec3(0.0);
        RayPayLoad.HitTexCoord      = vec2(0.0);
        RayPayLoad.MissEmissive     = vec3(0.0);
        RayPayLoad.HitMaterialIndex = 0;
        RayPayLoad.bFromInside      = false;
        
        // Trace-Ray
        traceRayEXT(uAccelerationStructure, gl_RayFlagsNoneEXT , 0xff, 0, 0, 0, Origin.xyz, MinT, Direction.xyz, MaxT, 0);

        const uint MaterialIndex = min(RayPayLoad.HitMaterialIndex, uScene.Settings.NumMaterials - 1);
        SMaterial Material = Materials[MaterialIndex];

        // Perform NormalMapping
        vec3 Normal;
        if (Material.NormalTexIndex != INVALID_BINDLESS_ID)
        {
            vec3 NormalMap = texture(uTextures[Material.NormalTexIndex], RayPayLoad.HitTexCoord).rgb;
            NormalMap = normalize(NormalMap * 2.0 - 1.0); // Transform from [0,1] range to [-1,1]
        
            const vec3 BiTangent = cross(RayPayLoad.HitNormal, RayPayLoad.HitTangent);
            const mat3 TBNMatrix = mat3(RayPayLoad.HitTangent, BiTangent, RayPayLoad.HitNormal);
            Normal = normalize(TBNMatrix * NormalMap);
        
            if (any(isnan(Normal)) || any(isinf(Normal)))
            {
                Normal = RayPayLoad.HitNormal;
            }
        }
        else
        {
            Normal = RayPayLoad.HitNormal;
        }

        if (RayPayLoad.bFromInside)
        {
            RayColor *= exp(-Material.AbsorbtionColor.rgb * RayPayLoad.HitT);
        }

        // Sample RoughnessTexture or just use the Specular Roughness in the material
        float SpecularRoughness;
        if (Material.RoughnessTexIndex != INVALID_BINDLESS_ID)
        {
            SpecularRoughness = texture(uTextures[Material.RoughnessTexIndex], RayPayLoad.HitTexCoord).r;
        }
        else
        {
            SpecularRoughness = Material.SpecularRoughness;
        }

        // Determine the chance of a specular ray
        float SpecularChance;
        if (Material.MetallicTexIndex != INVALID_BINDLESS_ID)
        {
            SpecularChance = texture(uTextures[Material.MetallicTexIndex], RayPayLoad.HitTexCoord).r;
        }
        else
        {
            SpecularChance = Material.SpecularChance;
        }

        // Take Fresnel into account
        float RefractionChance = Material.RefractionChance;
        if (SpecularChance > 0.0)
        {
            float IncidenceOfRefraction1 = RayPayLoad.bFromInside  ? Material.IncidenceOfRefraction : 1.0;
            float IncidenceOfRefraction2 = !RayPayLoad.bFromInside ? Material.IncidenceOfRefraction : 1.0;
            SpecularChance = FresnelReflectAmount(IncidenceOfRefraction1, IncidenceOfRefraction2, Direction, Normal, Material.SpecularChance, 1.0);

            float ChanceMultiplier = (1.0 - SpecularChance) / (1.0 - Material.SpecularChance);
            RefractionChance *= ChanceMultiplier;
        }

        // Calculate RayProbability
        float DoSpecular     = 0.0;
        float DoRefraction   = 0.0;
        float RayProbability = 1.0;
        float RaySelectRoll  = NextRandom(RandomSeed);
        if (SpecularChance > 0.0 && RaySelectRoll < SpecularChance)
        {
            DoSpecular     = 1.0;
            RayProbability = SpecularChance;
        }
        else if (RefractionChance > 0.0 && RaySelectRoll < (SpecularChance + RefractionChance))
        {
            DoRefraction   = 1.0;
            RayProbability = RefractionChance;
        }
        else
        {
            RayProbability = 1.0 - (SpecularChance + RefractionChance);
        }

        RayProbability = max(RayProbability, 0.001); 

        vec3 RayPosition  = vec3(0.0);
        vec3 RayDirection = Direction;
        if (DoRefraction == 1.0)
        {
            RayPosition = RayPayLoad.HitPosition - (Normal * RAY_OFFSET);
        }
        else
        {
            RayPosition = RayPayLoad.HitPosition + (Normal * RAY_OFFSET);
        }

        // Create diffuse ray
        vec3 DiffuseRay = normalize(Normal + NextRandomUnitSphereVec3(RandomSeed));

        // Create specular ray 
        vec3 SpecularRay = reflect(RayDirection, Normal);
        SpecularRay = normalize(mix(SpecularRay, DiffuseRay, SpecularRoughness * SpecularRoughness));
        
        // Create refraction ray
        vec3 RefractionRay = refract(RayDirection, Normal, RayPayLoad.bFromInside ? Material.IncidenceOfRefraction : 1.0 / Material.IncidenceOfRefraction);
        RefractionRay = normalize(mix(RefractionRay, normalize(Normal + NextRandomUnitSphereVec3(RandomSeed)), Material.RefractionRoughness * Material.RefractionRoughness));

        // Blend rays
        RayDirection = mix(DiffuseRay, SpecularRay, DoSpecular);
        RayDirection = mix(RayDirection, RefractionRay, DoRefraction);

        const vec3 Emissive = RayPayLoad.MissEmissive.rgb + Material.EmissiveColor.rgb; 
        SampleColor += Emissive * RayColor;

        if (DoRefraction == 0.0)
        {
            // Sample Material
            vec3 Albedo;
            if (Material.AlbedoTexIndex != INVALID_BINDLESS_ID)
            {
                Albedo = texture(uTextures[Material.AlbedoTexIndex], RayPayLoad.HitTexCoord).rgb;
            }
            else
            {
                Albedo = Material.AlbedoColor.rgb;
            }

            RayColor *= mix(Albedo.rgb, Material.SpecularColor.rgb, DoSpecular);
        }

        // Take ray probability into account
        RayColor /= RayProbability;

    #if ENABLE_RUSSIAN_ROULETTE
        float Probability = max(RayColor.r, max(RayColor.g, RayColor.b));
        if (NextRandom(RandomSeed) > Probability)
        {
            break;
        }

        // Add the energy we 'lose' by randomly terminating paths
        RayColor /= Probability;
    #endif

        // Setup next ray
        Origin    = RayPosition;
        Direction = RayDirection;
    }

    return SampleColor;
}

vec3 GetNormalForRay(in vec3 Origin, in vec3 Direction)
{
	float MinT = 0.001;
	float MaxT = 10000.0;

    RayPayLoad.HitNormal        = vec3(0.0);
    RayPayLoad.HitTangent       = vec3(0.0);
    RayPayLoad.HitPosition      = vec3(0.0);
    RayPayLoad.HitBarycentrics  = vec3(0.0);
    RayPayLoad.HitTexCoord      = vec2(0.0);
    RayPayLoad.MissEmissive     = vec3(0.0);
    RayPayLoad.HitMaterialIndex = -1;
    RayPayLoad.bFromInside      = false;
    
    // Trace-Ray
    traceRayEXT(uAccelerationStructure, gl_RayFlagsNoneEXT, 0xff, 0, 0, 0, Origin.xyz, MinT, Direction.xyz, MaxT, 0);

    if (RayPayLoad.HitMaterialIndex < 0)
    {
        return vec3(0.0);
    }

    const uint MaterialIndex = min(RayPayLoad.HitMaterialIndex, uScene.Settings.NumMaterials - 1);
    SMaterial Material = Materials[MaterialIndex];

    vec3 Normal;
    if (Material.NormalTexIndex != INVALID_BINDLESS_ID)
    {
        vec3 BiTangent = cross(RayPayLoad.HitNormal, RayPayLoad.HitTangent);
        vec3 NormalMap = texture(uTextures[Material.NormalTexIndex], RayPayLoad.HitTexCoord).rgb;
        NormalMap = normalize(NormalMap * 2.0 - 1.0); // Transform from [0,1] range to [-1,1]

        mat3 TBN = mat3(RayPayLoad.HitTangent, BiTangent, RayPayLoad.HitNormal);
        Normal = normalize(TBN * NormalMap);

        if (any(isnan(Normal)) || any(isinf(Normal)))
        {
            Normal = RayPayLoad.HitNormal;                 
        }
    }
    else
    {
        Normal = RayPayLoad.HitNormal;
    }

    // return (Normal + vec3(1.0)) * 0.5;
    return Normal;
}

vec3 GetBarycentricsForRay(in vec3 Origin, in vec3 Direction)
{
	float MinT = 0.001;
	float MaxT = 10000.0;

    RayPayLoad.HitNormal        = vec3(0.0);
    RayPayLoad.HitTangent       = vec3(0.0);
    RayPayLoad.HitPosition      = vec3(0.0);
    RayPayLoad.HitBarycentrics  = vec3(0.0);
    RayPayLoad.HitTexCoord      = vec2(0.0);
    RayPayLoad.MissEmissive     = vec3(0.0);
    RayPayLoad.HitMaterialIndex = -1;
    RayPayLoad.bFromInside      = false;
    
    // Trace-Ray
    traceRayEXT(uAccelerationStructure, gl_RayFlagsNoneEXT, 0xff, 0, 0, 0, Origin.xyz, MinT, Direction.xyz, MaxT, 0);

    if (RayPayLoad.HitMaterialIndex < 0)
    {
        return vec3(0.0);
    }
    else
    {
        return RayPayLoad.HitBarycentrics;
    }
}

vec3 GetTexCoordsForRay(in vec3 Origin, in vec3 Direction)
{
	float MinT = 0.001;
	float MaxT = 10000.0;

    RayPayLoad.HitNormal        = vec3(0.0);
    RayPayLoad.HitTangent       = vec3(0.0);
    RayPayLoad.HitPosition      = vec3(0.0);
    RayPayLoad.HitBarycentrics  = vec3(0.0);
    RayPayLoad.HitTexCoord      = vec2(0.0);
    RayPayLoad.MissEmissive     = vec3(0.0);
    RayPayLoad.HitMaterialIndex = -1;
    RayPayLoad.bFromInside      = false;
    
    // Trace-Ray
    traceRayEXT(uAccelerationStructure, gl_RayFlagsNoneEXT, 0xff, 0, 0, 0, Origin.xyz, MinT, Direction.xyz, MaxT, 0);

    if (RayPayLoad.HitMaterialIndex < 0)
    {
        return vec3(0.0);
    }
    else
    {
        return vec3(RayPayLoad.HitTexCoord, 0.0);
    }
}

vec3 GetAlbedoForRay(in vec3 Origin, in vec3 Direction)
{
	float MinT = 0.001;
	float MaxT = 10000.0;

    RayPayLoad.HitNormal        = vec3(0.0);
    RayPayLoad.HitTangent       = vec3(0.0);
    RayPayLoad.HitPosition      = vec3(0.0);
    RayPayLoad.HitBarycentrics  = vec3(0.0);
    RayPayLoad.HitTexCoord      = vec2(0.0);
    RayPayLoad.MissEmissive     = vec3(0.0);
    RayPayLoad.HitMaterialIndex = -1;
    RayPayLoad.bFromInside      = false;
    
    // Trace-Ray
    traceRayEXT(uAccelerationStructure, gl_RayFlagsNoneEXT, 0xff, 0, 0, 0, Origin.xyz, MinT, Direction.xyz, MaxT, 0);

    if (RayPayLoad.HitMaterialIndex < 0)
    {
        return vec3(0.0);
    }

    const uint MaterialIndex = min(RayPayLoad.HitMaterialIndex, uScene.Settings.NumMaterials - 1);
    SMaterial Material = Materials[MaterialIndex];
    if (Material.AlbedoTexIndex != INVALID_BINDLESS_ID)
    {
        return texture(uTextures[Material.AlbedoTexIndex], RayPayLoad.HitTexCoord).rgb;
    }
    else
    {
        return Material.AlbedoColor.rgb;
    }
}

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
	vec3 Origin    = vec3(uCamera.InverseView       * vec4(0.0, 0.0, 0.0, 1.0));
	vec3 Target    = vec3(uCamera.InverseProjection * vec4(d.x, d.y, 1.0, 1.0));
	vec3 Direction = vec3(uCamera.InverseView       * vec4(normalize(Target.xyz), 0.0));

    // Trace Rays
    if (uScene.Settings.ViewMode == VIEW_MODE_RENDER)
    {
        // Get Color for this Ray
        vec3 SampleColor = GetColorForRay(Origin, Direction, RandomSeed);

        // Accumulate samples over time
        vec4 PreviousColor = imageLoad(uPreviousFrame, ivec2(gl_LaunchIDEXT.xy));
        vec3 CurrentColor  = mix(PreviousColor.rgb, SampleColor, 1.0 / float(uRandom.FrameIndex + 1));
        imageStore(uOutput, ivec2(gl_LaunchIDEXT.xy), vec4(CurrentColor, 0.0));
    }
    else if (uScene.Settings.ViewMode == VIEW_MODE_NORMALS)
    {
        // Get Normal for this Ray
        vec3 SampleColor = GetNormalForRay(Origin, Direction);
        imageStore(uOutput, ivec2(gl_LaunchIDEXT.xy), vec4(SampleColor, 1.0));
    }
    else if (uScene.Settings.ViewMode == VIEW_MODE_ALBEDO)
    {
        // Get Albedo for this Ray
        vec3 SampleColor = GetAlbedoForRay(Origin, Direction);
        imageStore(uOutput, ivec2(gl_LaunchIDEXT.xy), vec4(SampleColor, 1.0));
    }
    else if (uScene.Settings.ViewMode == VIEW_MODE_BARYCENTRICS)
    {
        // Get Barycentrics for this Ray
        vec3 SampleColor = GetBarycentricsForRay(Origin, Direction);
        imageStore(uOutput, ivec2(gl_LaunchIDEXT.xy), vec4(SampleColor, 1.0));
    }
    else if (uScene.Settings.ViewMode == VIEW_MODE_TEXCOORDS)
    {
        // Get TexCoords for this Ray
        vec3 SampleColor = GetTexCoordsForRay(Origin, Direction);
        imageStore(uOutput, ivec2(gl_LaunchIDEXT.xy), vec4(SampleColor, 1.0));
    }
}
