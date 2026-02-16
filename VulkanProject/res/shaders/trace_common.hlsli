#ifndef TRACE_COMMON_HLSLI
#define TRACE_COMMON_HLSLI

float3 SafeNormalize(float3 Value, float3 Fallback)
{
    if (any(isnan(Value)) || any(isinf(Value)))
    {
        return Fallback;
    }

    const float LengthSq = dot(Value, Value);
    if (LengthSq < 1e-8)
    {
        return Fallback;
    }

    return Value * rsqrt(LengthSq);
}

float3 EncodeDirectionForView(float3 Value)
{
    // Keep debug vectors in signed space [-1, 1] to preserve direction.
    return SafeNormalize(Value, float3(0.0, 0.0, 1.0));
}

float3 EncodeNormal(float3 Value)
{
    return EncodeDirectionForView(Value);
}

float3 EncodeGeometricNormal(float3 Value)
{
    return EncodeDirectionForView(Value);
}

float3 EncodeTangent(float3 Value)
{
    return EncodeDirectionForView(Value);
}

float3 CalculateFilmTarget(float3 CameraPosition, float3 CameraForward, float FieldOfViewDegrees, float2 PixelCoord, float2 Size, float2 Jitter)
{
    float3 CamForward = normalize(CameraForward);
    float3 CamUp      = float3(0.0, 1.0, 0.0);
    CamUp = normalize(CamUp - dot(CamUp, CamForward) * CamForward);
    float3 CamRight = normalize(cross(CamUp, CamForward));

    float  AspectRatio  = Size.x / Size.y;
    float  FieldOfView  = clamp(FieldOfViewDegrees, 30.0, 120.0);
    float  FilmDistance = 1.0 / tan(FieldOfView * 0.5 * 3.1415926535 / 180.0);
    float3 FilmCenter   = CameraPosition + (CamForward * FilmDistance);

    float2 FilmUV = (PixelCoord + Jitter) / Size;
    FilmUV.y = 1.0 - FilmUV.y;
    FilmUV   = FilmUV * 2.0;

    float2 FilmCoord = float2(-1.0, -1.0) + FilmUV;
    FilmCoord.x = FilmCoord.x * AspectRatio;

    return FilmCenter + (CamRight * FilmCoord.x) + (CamUp * FilmCoord.y);
}

float3 GetEnvironmentLight(uint BackgroundType, float GradientLightStrength, TextureCube<float4> SkyboxTexture, SamplerState SkyboxSampler, float3 RayDirection, float SkyboxMultiplier)
{
    if (BackgroundType == 0)
    {
        return float3(0.0, 0.0, 0.0);
    }
    else if (BackgroundType == 1)
    {
        float3 UnitDir  = normalize(RayDirection);
        float  Alpha    = 0.5 * (UnitDir.y + 1.0);
        float3 Color    = (1.0 - Alpha) * float3(1.0, 1.0, 1.0) + Alpha * float3(0.5, 0.7, 1.0);
        float  Strength = max(1.0, GradientLightStrength);
        return Color * Strength;
    }
    else if (BackgroundType == 2)
    {
        float3 UnitDirection = normalize(RayDirection);
        float4 SkyboxColor   = SkyboxTexture.SampleLevel(SkyboxSampler, UnitDirection, 0.0);
        return SkyboxColor.rgb * SkyboxMultiplier;
    }

    return float3(0.0, 0.0, 0.0);
}

#ifndef TRACE_COMMON_NO_TRACE_API

struct SRayDesc
{
    float3 Origin;
    float3 Direction;
    float  MinT;
    float  MaxT;
};

struct STraceHit
{
    float3 Normal;
    float3 Tangent;
    float3 Position;
    float3 Barycentrics;
    float3 MissEmissive;
    float2 TexCoords;
    uint   MaterialIndex;
    float  HitT;
    uint   FromInside;
};

bool BackendTraceRay(SRayDesc RayInput, out STraceHit Hit, inout int2 Stats);
SMaterial BackendGetMaterial(uint MaterialIndex);
uint BackendGetNumMaterials();

bool TracePathRay(SRayDesc RayInput, out STraceHit Hit, inout int2 Stats)
{
    Hit.Normal        = float3(0.0, 0.0, 0.0);
    Hit.Tangent       = float3(0.0, 0.0, 0.0);
    Hit.Position      = float3(0.0, 0.0, 0.0);
    Hit.Barycentrics  = float3(0.0, 0.0, 0.0);
    Hit.MissEmissive  = float3(0.0, 0.0, 0.0);
    Hit.TexCoords     = float2(0.0, 0.0);
    Hit.MaterialIndex = 0xFFFFFFFFu;
    Hit.HitT          = -1.0;
    Hit.FromInside   = 0;

    return BackendTraceRay(RayInput, Hit, Stats);
}

float3 GetColorForRay(SRayDesc RayInput, inout uint RandomSeed, uint NumBounces)
{
    float3 RayColor    = float3(1.0, 1.0, 1.0);
    float3 SampleColor = float3(0.0, 0.0, 0.0);

    for (uint i = 0; i < NumBounces; i++)
    {
        int2 Stats = int2(0, 0);

        STraceHit Hit;
        if (!TracePathRay(RayInput, Hit, Stats))
        {
            SampleColor += Hit.MissEmissive * RayColor;
            break;
        }

        const uint NumMaterials = BackendGetNumMaterials();
        if (NumMaterials == 0)
        {
            break;
        }

        const uint MaterialIndex = min(Hit.MaterialIndex, NumMaterials - 1);
        const SMaterial Material = BackendGetMaterial(MaterialIndex);

        SShadingInput ShadingInput;
        ShadingInput.Normal      = Hit.Normal;
        ShadingInput.Tangent     = Hit.Tangent;
        ShadingInput.Position    = Hit.Position;
        ShadingInput.TexCoords   = Hit.TexCoords;
        ShadingInput.HitT        = Hit.HitT;
        ShadingInput.FromInside = Hit.FromInside;

        float3 RayPosition;
        float3 RayDirection;
        
        uint HasValidRay = 0;
        ShadeSurface(Material, ShadingInput, RayInput.Direction, Hit.MissEmissive + Material.EmissiveColor.rgb, RAY_OFFSET, RandomSeed, RayColor, SampleColor, RayPosition, RayDirection, HasValidRay);
        if (HasValidRay == 0)
        {
            break;
        }

        RayInput.Origin    = RayPosition;
        RayInput.Direction = RayDirection;
    }

    return SampleColor;
}

float3 GetNormalForRay(SRayDesc RayInput)
{
    int2 Stats = int2(0, 0);

    STraceHit Hit;
    if (!TracePathRay(RayInput, Hit, Stats))
    {
        return float3(0.0, 0.0, 0.0);
    }

    const uint NumMaterials = BackendGetNumMaterials();
    if (NumMaterials == 0)
    {
        return float3(0.0, 0.0, 0.0);
    }

    const uint MaterialIndex = min(Hit.MaterialIndex, NumMaterials - 1);
    const SMaterial Material = BackendGetMaterial(MaterialIndex);

    SShadingInput ShadingInput;
    ShadingInput.Normal      = Hit.Normal;
    ShadingInput.Tangent     = Hit.Tangent;
    ShadingInput.Position    = Hit.Position;
    ShadingInput.TexCoords   = Hit.TexCoords;
    ShadingInput.HitT        = Hit.HitT;
    ShadingInput.FromInside = Hit.FromInside;

    const float3 Normal = GetShadingNormal(Material, ShadingInput);
    return EncodeNormal(Normal);
}

float3 GetGeometricNormalForRay(SRayDesc RayInput)
{
    int2 Stats = int2(0, 0);
    
    STraceHit Hit;
    if (!TracePathRay(RayInput, Hit, Stats))
    {
        return float3(0.0, 0.0, 0.0);
    }

    return EncodeGeometricNormal(SafeNormalize(Hit.Normal, float3(0.0, 0.0, 1.0)));
}

float3 GetTangentForRay(SRayDesc RayInput)
{
    int2 Stats = int2(0, 0);

    STraceHit Hit;
    if (!TracePathRay(RayInput, Hit, Stats))
    {
        return float3(0.0, 0.0, 0.0);
    }

    return EncodeTangent(Hit.Tangent);
}

float3 GetBarycentricsForRay(SRayDesc RayInput)
{
    int2 Stats = int2(0, 0);

    STraceHit Hit;
    if (!TracePathRay(RayInput, Hit, Stats))
    {
        return float3(0.0, 0.0, 0.0);
    }

    if (any(isnan(Hit.Barycentrics)) || any(isinf(Hit.Barycentrics)))
    {
        return float3(0.0, 0.0, 0.0);
    }

    return saturate(Hit.Barycentrics);
}

float3 GetTexCoordsForRay(SRayDesc RayInput)
{
    int2 Stats = int2(0, 0);

    STraceHit Hit;
    if (!TracePathRay(RayInput, Hit, Stats))
    {
        return float3(0.0, 0.0, 0.0);
    }

    if (any(isnan(Hit.TexCoords)) || any(isinf(Hit.TexCoords)))
    {
        return float3(0.0, 0.0, 0.0);
    }

    return float3(frac(Hit.TexCoords), 0.0);
}

float3 GetAlbedoForRay(SRayDesc RayInput)
{
    int2 Stats = int2(0, 0);

    STraceHit Hit;
    if (!TracePathRay(RayInput, Hit, Stats))
    {
        return float3(0.0, 0.0, 0.0);
    }

    const uint NumMaterials = BackendGetNumMaterials();
    if (NumMaterials == 0)
    {
        return float3(0.0, 0.0, 0.0);
    }

    const uint MaterialIndex = min(Hit.MaterialIndex, NumMaterials - 1);
    const SMaterial Material = BackendGetMaterial(MaterialIndex);
    return saturate(GetMaterialAlbedo(Material, Hit.TexCoords));
}

#if ENABLE_SOFTWARE_TRACING
float3 GetSoftwareBvhIntersectionColor(SRayDesc RayDesc);
#endif

float3 EvaluateViewModeColor(uint ViewMode, SRayDesc RayDesc, inout uint RandomSeed, uint NumBounces, out bool Accumulate)
{
    Accumulate = (ViewMode == 0);
    if (ViewMode == 0)
    {
        return GetColorForRay(RayDesc, RandomSeed, NumBounces);
    }
    else if (ViewMode == 1)
    {
        return GetNormalForRay(RayDesc);
    }
    else if (ViewMode == 2)
    {
        return GetGeometricNormalForRay(RayDesc);
    }
    else if (ViewMode == 3)
    {
        return GetTangentForRay(RayDesc);
    }
    else if (ViewMode == 4)
    {
        return GetAlbedoForRay(RayDesc);
    }
    else if (ViewMode == 5)
    {
        return GetBarycentricsForRay(RayDesc);
    }
    else if (ViewMode == 6)
    {
        return GetTexCoordsForRay(RayDesc);
    }
#if ENABLE_SOFTWARE_TRACING
    else if (ViewMode == 7)
    {
        Accumulate = false;
        return GetSoftwareBvhIntersectionColor(RayDesc);
    }
#endif

    Accumulate = false;
    return float3(0.0, 0.0, 0.0);
}

void WriteViewModeOutput(bool WritePrimary, uint2 PixelCoord, float3 SampleColor, bool Accumulate, uint FrameIndex, RWTexture2D<float4> OutputTexture, RWTexture2D<float4> PreviousFrameTexture)
{
    float3 ColorToWrite = SampleColor;
    if (Accumulate)
    {
        float4 PreviousColor = WritePrimary ? PreviousFrameTexture[PixelCoord] : OutputTexture[PixelCoord];
        ColorToWrite = lerp(PreviousColor.rgb, SampleColor, 1.0 / (float)(FrameIndex + 1));
    }

    if (WritePrimary)
    {
        OutputTexture[PixelCoord] = float4(ColorToWrite, 1.0);
    }
    else
    {
        PreviousFrameTexture[PixelCoord] = float4(ColorToWrite, 1.0);
    }
}

#endif

#endif

