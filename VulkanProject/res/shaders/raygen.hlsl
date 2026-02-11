#include "hw_ray_trace_common.hlsli"
#include "random.hlsli"
#include "primitives.hlsli"

#define RAY_OFFSET 0.001
#define MAX_DEPTH 1024

#define VIEW_MODE_RENDER 0
#define VIEW_MODE_NORMALS 1
#define VIEW_MODE_GEOMETRIC_NORMALS 2
#define VIEW_MODE_TANGENTS 3
#define VIEW_MODE_ALBEDO 4
#define VIEW_MODE_BARYCENTRICS 5
#define VIEW_MODE_TEXCOORDS 6

#define ENABLE_RUSSIAN_ROULETTE 1
#define HW_RAY_FLAGS RAY_FLAG_CULL_BACK_FACING_TRIANGLES

[[vk::binding(0)]] RaytracingAccelerationStructure uAccelerationStructure;
[[vk::binding(1)]] RWTexture2D<float4> uOutput;
[[vk::binding(2)]] RWTexture2D<float4> uPreviousFrame;

struct SCameraBuffer
{
    float4x4 Projection;
    float4x4 View;
    float4x4 InverseProjection;
    float4x4 InverseView;
    float4   Position;
    float4   Forward;
    float    FieldOfViewDegrees;
    // Padding
    uint     Padding0;
    uint     Padding1;
    uint     Padding2;
};

struct SRandomBuffer
{
    uint FrameIndex;
    uint HaltonIndex;
    // Padding
    uint Padding0;
    uint Padding1;
};

struct SSceneBuffer
{
    SSceneSettings Settings;
};

[[vk::binding(3)]]
ConstantBuffer<SCameraBuffer> uCamera;

[[vk::binding(4)]]
ConstantBuffer<SRandomBuffer> uRandom;

[[vk::binding(5)]]
ConstantBuffer<SSceneBuffer> uScene;

[[vk::binding(7)]]
StructuredBuffer<SMaterial> MaterialBuffer;

[[vk::binding(0, 1)]] Texture2D<float4> uTextures[];
[[vk::binding(0, 1)]] SamplerState uTexturesSampler : register(s0, space1);

float FresnelReflectAmount(float N1, float N2, float3 Normal, float3 Incident, float F0, float F90);

float3 GetColorForRay(float3 Origin, float3 Direction, inout uint RandomSeed, RayDesc rayDesc);
float3 GetNormalForRay(float3 Origin, float3 Direction, RayDesc rayDesc);
float3 GetGeometricNormalForRay(float3 Origin, float3 Direction, RayDesc rayDesc);
float3 GetTangentForRay(float3 Origin, float3 Direction, RayDesc rayDesc);
float3 GetBarycentricsForRay(float3 Origin, float3 Direction, RayDesc rayDesc);
float3 GetTexCoordsForRay(float3 Origin, float3 Direction, RayDesc rayDesc);
float3 GetAlbedoForRay(float3 Origin, float3 Direction, RayDesc rayDesc);

[shader("raygeneration")]
void main()
{
    uint3 launchId   = DispatchRaysIndex();
    uint3 launchSize = DispatchRaysDimensions();
    uint  RandomSeed = InitRandom(launchId.xy, launchSize.x, uRandom.FrameIndex);

    float2 Jitter      = float2(NextRandom(RandomSeed), NextRandom(RandomSeed)) - 0.5;
    float2 PixelCenter = float2(launchId.xy) + float2(0.5, 0.5);

    float3 Origin     = uCamera.Position.xyz;
    float3 CamForward = normalize(uCamera.Forward.xyz);
    float3 CamUp      = float3(0.0, 1.0, 0.0);
    CamUp = normalize(CamUp - dot(CamUp, CamForward) * CamForward);

    float3 CamRight     = normalize(cross(CamUp, CamForward));
    float  AspectRatio  = (float)launchSize.x / (float)launchSize.y;
    float  FieldOfView  = clamp(uCamera.FieldOfViewDegrees, 30.0, 120.0);
    float  FilmDistance = 1.0 / tan(FieldOfView * 0.5 * 3.1415926535 / 180.0);
    float3 FilmCenter   = Origin + (CamForward * FilmDistance);

    float2 FilmUV = (PixelCenter + Jitter) / float2(launchSize.xy);
    FilmUV.y = 1.0 - FilmUV.y;
    FilmUV *= 2.0;

    float2 FilmCoord = float2(-1.0, -1.0) + FilmUV;
    FilmCoord.x *= AspectRatio;
    
    float3 Target    = FilmCenter + (CamRight * FilmCoord.x) + (CamUp * FilmCoord.y);
    float3 Direction = normalize(Target - Origin);

    RayDesc rayDesc;
    rayDesc.Origin    = Origin;
    rayDesc.Direction = Direction;
    rayDesc.TMin      = 0.001;
    rayDesc.TMax      = 10000.0;

    float3 SampleColor = float3(0.0, 0.0, 0.0);
    if (uScene.Settings.ViewMode == VIEW_MODE_RENDER)
    {
        SampleColor = GetColorForRay(Origin, Direction, RandomSeed, rayDesc);

        float4 PreviousColor = uPreviousFrame[launchId.xy];
        float3 CurrentColor  = lerp(PreviousColor.rgb, SampleColor, 1.0 / (float)(uRandom.FrameIndex + 1));
        uOutput[launchId.xy] = float4(CurrentColor, 1.0);
    }
    else if (uScene.Settings.ViewMode == VIEW_MODE_NORMALS)
    {
        SampleColor = GetNormalForRay(Origin, Direction, rayDesc);
        uOutput[launchId.xy] = float4(SampleColor, 1.0);
    }
    else if (uScene.Settings.ViewMode == VIEW_MODE_GEOMETRIC_NORMALS)
    {
        SampleColor = GetGeometricNormalForRay(Origin, Direction, rayDesc);
        uOutput[launchId.xy] = float4(SampleColor, 1.0);
    }
    else if (uScene.Settings.ViewMode == VIEW_MODE_TANGENTS)
    {
        SampleColor = GetTangentForRay(Origin, Direction, rayDesc);
        uOutput[launchId.xy] = float4(SampleColor, 1.0);
    }
    else if (uScene.Settings.ViewMode == VIEW_MODE_ALBEDO)
    {
        SampleColor = GetAlbedoForRay(Origin, Direction, rayDesc);
        uOutput[launchId.xy] = float4(SampleColor, 1.0);
    }
    else if (uScene.Settings.ViewMode == VIEW_MODE_BARYCENTRICS)
    {
        SampleColor = GetBarycentricsForRay(Origin, Direction, rayDesc);
        uOutput[launchId.xy] = float4(SampleColor, 1.0);
    }
    else if (uScene.Settings.ViewMode == VIEW_MODE_TEXCOORDS)
    {
        SampleColor = GetTexCoordsForRay(Origin, Direction, rayDesc);
        uOutput[launchId.xy] = float4(SampleColor, 1.0);
    }
}

float FresnelReflectAmount(float N1, float N2, float3 Normal, float3 Incident, float F0, float F90)
{
    float R0 = (N1 - N2) / (N1 + N2);
    R0 *= R0;

    float CosX = -dot(Normal, Incident);
    if (N1 > N2)
    {
        float N = N1 / N2;
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
    return lerp(F0, F90, Result);
}

float3 GetColorForRay(float3 Origin, float3 Direction, inout uint RandomSeed, RayDesc rayDesc)
{
    float3 RayColor    = float3(1.0, 1.0, 1.0);
    float3 SampleColor = float3(0.0, 0.0, 0.0);

    const uint NumBounces = min(uScene.Settings.NumBounces, MAX_DEPTH);
    for (uint i = 0; i < NumBounces; i++)
    {
        SRayPayLoad RayPayLoad;
        RayPayLoad.HitNormal        = float3(0.0, 0.0, 0.0);
        RayPayLoad.HitTangent       = float3(0.0, 0.0, 0.0);
        RayPayLoad.HitPosition      = float3(0.0, 0.0, 0.0);
        RayPayLoad.HitBarycentrics  = float3(0.0, 0.0, 0.0);
        RayPayLoad.MissEmissive     = float3(0.0, 0.0, 0.0);
        RayPayLoad.HitMaterialIndex = 0;
        RayPayLoad.HitT             = -1.0;
        RayPayLoad.bFromInside      = 0;

        TraceRay(uAccelerationStructure, HW_RAY_FLAGS, 0xff, 0, 0, 0, rayDesc, RayPayLoad);

        if (RayPayLoad.HitT < 0.0)
        {
            SampleColor += RayPayLoad.MissEmissive * RayColor;
            break;
        }

        const uint MaterialIndex = min(RayPayLoad.HitMaterialIndex, uScene.Settings.NumMaterials - 1);
        SMaterial Material = MaterialBuffer[MaterialIndex];

        float3 Normal;
        if (Material.NormalTexIndex != INVALID_BINDLESS_ID)
        {
            float3 NormalMap = uTextures[Material.NormalTexIndex].SampleLevel(uTexturesSampler, RayPayLoad.HitTexCoord, 0.0).rgb;
            NormalMap = normalize(NormalMap * 2.0 - 1.0);
            
            float3 BiTangent = cross(RayPayLoad.HitNormal, RayPayLoad.HitTangent);

            float3x3 TBNMatrix = float3x3(RayPayLoad.HitTangent, BiTangent, RayPayLoad.HitNormal);
            Normal = normalize(mul(NormalMap, TBNMatrix));

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

        float SpecularRoughness;
        if (Material.RoughnessTexIndex != INVALID_BINDLESS_ID)
        {
            SpecularRoughness = uTextures[Material.RoughnessTexIndex].SampleLevel(uTexturesSampler, RayPayLoad.HitTexCoord, 0.0).r;
        }
        else
        {
            SpecularRoughness = Material.SpecularRoughness;
        }

        float SpecularChance;
        if (Material.MetallicTexIndex != INVALID_BINDLESS_ID)
        {
            SpecularChance = uTextures[Material.MetallicTexIndex].SampleLevel(uTexturesSampler, RayPayLoad.HitTexCoord, 0.0).r;
        }
        else
        {
            SpecularChance = Material.SpecularChance;
        }

        float RefractionChance = Material.RefractionChance;
        if ((SpecularChance > 0.0) || (RefractionChance > 0.0))
        {
            float IncidenceOfRefraction1 = RayPayLoad.bFromInside ? Material.IncidenceOfRefraction : 1.0;
            float IncidenceOfRefraction2 = RayPayLoad.bFromInside ? 1.0 : Material.IncidenceOfRefraction;
            SpecularChance = FresnelReflectAmount(IncidenceOfRefraction1, IncidenceOfRefraction2, Direction, Normal, Material.SpecularChance, 1.0);

            float ChanceMultiplier = (1.0 - SpecularChance) / max(1.0 - Material.SpecularChance, 0.001);
            RefractionChance *= ChanceMultiplier;
        }

        SpecularChance   = saturate(SpecularChance);
        RefractionChance = saturate(RefractionChance);
        
        if (SpecularChance + RefractionChance > 1.0)
        {
            RefractionChance = 1.0 - SpecularChance;
        }

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

        float3 RayDirection = Direction;
        float3 DiffuseRay   = normalize(Normal + NextRandomUnitSphereVec3(RandomSeed));
        float3 SpecularRay  = reflect(RayDirection, Normal);
        SpecularRay = normalize(lerp(SpecularRay, DiffuseRay, SpecularRoughness * SpecularRoughness));

        float3 RefractionRay = refract(RayDirection, Normal, RayPayLoad.bFromInside ? Material.IncidenceOfRefraction : (1.0 / Material.IncidenceOfRefraction));
        if (DoRefraction == 1.0 && dot(RefractionRay, RefractionRay) < 1e-8)
        {
            DoRefraction   = 0.0;
            DoSpecular     = 1.0;
            RayProbability = max(SpecularChance, 0.001);
            RefractionRay  = SpecularRay;
        }

        RefractionRay = normalize(lerp(RefractionRay, normalize(Normal + NextRandomUnitSphereVec3(RandomSeed)), Material.RefractionRoughness * Material.RefractionRoughness));
        RayDirection  = lerp(DiffuseRay, SpecularRay, DoSpecular);
        RayDirection  = lerp(RayDirection, RefractionRay, DoRefraction);

        if (any(isnan(RayDirection)) || any(isinf(RayDirection)) || dot(RayDirection, RayDirection) < 1e-8)
        {
            break;
        }

        RayDirection = normalize(RayDirection);
        float3 RayPosition = RayPayLoad.HitPosition + (RayDirection * RAY_OFFSET);

        const float3 Emissive = RayPayLoad.MissEmissive.rgb + Material.EmissiveColor.rgb;
        SampleColor += Emissive * RayColor;

        if (DoRefraction == 0.0)
        {
            float3 Albedo;
            if (Material.AlbedoTexIndex != INVALID_BINDLESS_ID)
            {
                Albedo = uTextures[Material.AlbedoTexIndex].SampleLevel(uTexturesSampler, RayPayLoad.HitTexCoord, 0.0).rgb;
            }
            else
            {
                Albedo = Material.AlbedoColor.rgb;
            }

            RayColor *= lerp(Albedo.rgb, Material.SpecularColor.rgb, DoSpecular);
        }

        RayColor /= RayProbability;

#if ENABLE_RUSSIAN_ROULETTE
        float Probability = max(RayColor.r, max(RayColor.g, RayColor.b));
        if (NextRandom(RandomSeed) > Probability)
        {
            break;
        }

        RayColor /= Probability;
#endif

        Origin            = RayPosition;
        Direction         = RayDirection;
        rayDesc.Origin    = RayPosition;
        rayDesc.Direction = RayDirection;
    }

    return SampleColor;
}

float3 GetNormalForRay(float3 Origin, float3 Direction, RayDesc rayDesc)
{
    SRayPayLoad RayPayLoad;
    RayPayLoad.HitNormal        = float3(0.0, 0.0, 0.0);
    RayPayLoad.HitTangent       = float3(0.0, 0.0, 0.0);
    RayPayLoad.HitPosition      = float3(0.0, 0.0, 0.0);
    RayPayLoad.HitBarycentrics  = float3(0.0, 0.0, 0.0);
    RayPayLoad.HitTexCoord      = float2(0.0, 0.0);
    RayPayLoad.MissEmissive     = float3(0.0, 0.0, 0.0);
    RayPayLoad.HitMaterialIndex = 0xFFFFFFFFu;
    RayPayLoad.bFromInside      = 0;

    TraceRay(uAccelerationStructure, HW_RAY_FLAGS, 0xff, 0, 0, 0, rayDesc, RayPayLoad);

    if (RayPayLoad.HitMaterialIndex == 0xFFFFFFFFu)
    {
        return float3(0.0, 0.0, 0.0);
    }

    const uint MaterialIndex = min(RayPayLoad.HitMaterialIndex, uScene.Settings.NumMaterials - 1);
    SMaterial Material = MaterialBuffer[MaterialIndex];

    float3 Normal;
    if (Material.NormalTexIndex != INVALID_BINDLESS_ID)
    {
        float3 NormalMap = uTextures[Material.NormalTexIndex].SampleLevel(uTexturesSampler, RayPayLoad.HitTexCoord, 0.0).rgb;
        NormalMap = normalize(NormalMap * 2.0 - 1.0);
        
        float3 BiTangent = cross(RayPayLoad.HitNormal, RayPayLoad.HitTangent);

        float3x3 TBN = float3x3(RayPayLoad.HitTangent, BiTangent, RayPayLoad.HitNormal);
        Normal = normalize(mul(NormalMap, TBN));

        if (any(isnan(Normal)) || any(isinf(Normal)))
        {
            Normal = RayPayLoad.HitNormal;
        }
    }
    else
    {
        Normal = RayPayLoad.HitNormal;
    }

    return Normal;
}

float3 GetGeometricNormalForRay(float3 Origin, float3 Direction, RayDesc rayDesc)
{
    SRayPayLoad RayPayLoad;
    RayPayLoad.HitNormal        = float3(0.0, 0.0, 0.0);
    RayPayLoad.HitTangent       = float3(0.0, 0.0, 0.0);
    RayPayLoad.HitPosition      = float3(0.0, 0.0, 0.0);
    RayPayLoad.HitBarycentrics  = float3(0.0, 0.0, 0.0);
    RayPayLoad.HitTexCoord      = float2(0.0, 0.0);
    RayPayLoad.MissEmissive     = float3(0.0, 0.0, 0.0);
    RayPayLoad.HitMaterialIndex = 0xFFFFFFFFu;
    RayPayLoad.bFromInside      = 0;

    TraceRay(uAccelerationStructure, HW_RAY_FLAGS, 0xff, 0, 0, 0, rayDesc, RayPayLoad);

    if (RayPayLoad.HitMaterialIndex == 0xFFFFFFFFu)
    {
        return float3(0.0, 0.0, 0.0);
    }

    return normalize(RayPayLoad.HitNormal);
}

float3 GetTangentForRay(float3 Origin, float3 Direction, RayDesc rayDesc)
{
    SRayPayLoad RayPayLoad;
    RayPayLoad.HitNormal        = float3(0.0, 0.0, 0.0);
    RayPayLoad.HitTangent       = float3(0.0, 0.0, 0.0);
    RayPayLoad.HitPosition      = float3(0.0, 0.0, 0.0);
    RayPayLoad.HitBarycentrics  = float3(0.0, 0.0, 0.0);
    RayPayLoad.HitTexCoord      = float2(0.0, 0.0);
    RayPayLoad.MissEmissive     = float3(0.0, 0.0, 0.0);
    RayPayLoad.HitMaterialIndex = 0xFFFFFFFFu;
    RayPayLoad.bFromInside      = 0;

    TraceRay(uAccelerationStructure, HW_RAY_FLAGS, 0xff, 0, 0, 0, rayDesc, RayPayLoad);

    if (RayPayLoad.HitMaterialIndex == 0xFFFFFFFFu)
    {
        return float3(0.0, 0.0, 0.0);
    }

    return RayPayLoad.HitTangent;
}

float3 GetBarycentricsForRay(float3 Origin, float3 Direction, RayDesc rayDesc)
{
    SRayPayLoad RayPayLoad;
    RayPayLoad.HitMaterialIndex = 0xFFFFFFFFu;

    TraceRay(uAccelerationStructure, HW_RAY_FLAGS, 0xff, 0, 0, 0, rayDesc, RayPayLoad);

    if (RayPayLoad.HitMaterialIndex == 0xFFFFFFFFu)
    {
        return float3(0.0, 0.0, 0.0);
    }

    return RayPayLoad.HitBarycentrics;
}

float3 GetTexCoordsForRay(float3 Origin, float3 Direction, RayDesc rayDesc)
{
    SRayPayLoad RayPayLoad;
    RayPayLoad.HitMaterialIndex = 0xFFFFFFFFu;

    TraceRay(uAccelerationStructure, HW_RAY_FLAGS, 0xff, 0, 0, 0, rayDesc, RayPayLoad);

    if (RayPayLoad.HitMaterialIndex == 0xFFFFFFFFu)
    {
        return float3(0.0, 0.0, 0.0);
    }

    return float3(RayPayLoad.HitTexCoord, 0.0);
}

float3 GetAlbedoForRay(float3 Origin, float3 Direction, RayDesc rayDesc)
{
    SRayPayLoad RayPayLoad;
    RayPayLoad.HitMaterialIndex = 0xFFFFFFFFu;

    TraceRay(uAccelerationStructure, HW_RAY_FLAGS, 0xff, 0, 0, 0, rayDesc, RayPayLoad);

    if (RayPayLoad.HitMaterialIndex == 0xFFFFFFFFu)
    {
        return float3(0.0, 0.0, 0.0);
    }

    const uint MaterialIndex = min(RayPayLoad.HitMaterialIndex, uScene.Settings.NumMaterials - 1);
    SMaterial Material = MaterialBuffer[MaterialIndex];

    if (Material.AlbedoTexIndex != INVALID_BINDLESS_ID)
    {
        return uTextures[Material.AlbedoTexIndex].SampleLevel(uTexturesSampler, RayPayLoad.HitTexCoord, 0.0).rgb;
    }
    else
    {
        return Material.AlbedoColor.rgb;
    }
}
