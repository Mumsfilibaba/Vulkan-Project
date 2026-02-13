#ifndef SHADING_HLSLI
#define SHADING_HLSLI

struct SShadingInput
{
    float3 Normal;
    float3 Tangent;
    float3 Position;
    float2 TexCoords;
    float  HitT;
    uint   FromInside;
};

float FresnelReflectAmount(float N1, float N2, float3 Normal, float3 Incident, float F0, float F90)
{
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
    return lerp(F0, F90, Result);
}

float3 GetShadingNormal(SMaterial Material, SShadingInput Input)
{
    if (Material.NormalTexIndex == INVALID_BINDLESS_ID)
    {
        return Input.Normal;
    }

    float3 NormalMap = Textures[Material.NormalTexIndex].SampleLevel(TexturesSampler, Input.TexCoords, 0.0).rgb;
    NormalMap = normalize(NormalMap * 2.0 - 1.0);

    const float3   Bitangent = cross(Input.Normal, Input.Tangent);
    const float3x3 TBNMatrix = float3x3(Input.Tangent, Bitangent, Input.Normal);

    float3 Normal = normalize(mul(NormalMap, TBNMatrix));
    if (any(isnan(Normal)) || any(isinf(Normal)))
    {
        return Input.Normal;
    }

    return Normal;
}

float3 GetMaterialAlbedo(SMaterial Material, float2 TexCoords)
{
    if (Material.AlbedoTexIndex != INVALID_BINDLESS_ID)
    {
        return Textures[Material.AlbedoTexIndex].SampleLevel(TexturesSampler, TexCoords, 0.0).rgb;
    }

    return Material.AlbedoColor.rgb;
}

void ShadeSurface(SMaterial Material, SShadingInput Input, float3 IncomingDirection, float3 EmissiveContribution, float RayOffset, inout uint RandomSeed,
    inout float3 RayColor, inout float3 SampleColor, out float3 OutRayOrigin, out float3 OutRayDirection, out uint HasValidRay)
{
    float3 Normal = GetShadingNormal(Material, Input);

    if (Input.FromInside != 0)
    {
        RayColor *= exp(-Material.AbsorbtionColor.rgb * Input.HitT);
    }

    float SpecularRoughness;
    if (Material.RoughnessTexIndex != INVALID_BINDLESS_ID)
    {
        SpecularRoughness = Textures[Material.RoughnessTexIndex].SampleLevel(TexturesSampler, Input.TexCoords, 0.0).r;
    }
    else
    {
        SpecularRoughness = Material.SpecularRoughness;
    }

    float SpecularChance;
    if (Material.MetallicTexIndex != INVALID_BINDLESS_ID)
    {
        SpecularChance = Textures[Material.MetallicTexIndex].SampleLevel(TexturesSampler, Input.TexCoords, 0.0).r;
    }
    else
    {
        SpecularChance = Material.SpecularChance;
    }

    float RefractionChance = Material.RefractionChance;
    if ((SpecularChance > 0.0) || (RefractionChance > 0.0))
    {
        float IncidenceOfRefraction1 = (Input.FromInside != 0) ? Material.IncidenceOfRefraction : 1.0;
        float IncidenceOfRefraction2 = (Input.FromInside != 0) ? 1.0 : Material.IncidenceOfRefraction;
        SpecularChance = FresnelReflectAmount(IncidenceOfRefraction1, IncidenceOfRefraction2, IncomingDirection, Normal, Material.SpecularChance, 1.0);

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

    float3 DiffuseRay  = normalize(Normal + NextRandomUnitSphereVec3(RandomSeed));
    float3 SpecularRay = reflect(IncomingDirection, Normal);
    SpecularRay        = normalize(lerp(SpecularRay, DiffuseRay, SpecularRoughness * SpecularRoughness));

    float  RefractionRatio = (Input.FromInside != 0) ? Material.IncidenceOfRefraction : (1.0 / Material.IncidenceOfRefraction);
    float3 RefractionRay   = refract(IncomingDirection, Normal, RefractionRatio);
    
    if (DoRefraction == 1.0 && dot(RefractionRay, RefractionRay) < 1e-8)
    {
        DoRefraction   = 0.0;
        DoSpecular     = 1.0;
        RayProbability = max(SpecularChance, 0.001);
        RefractionRay  = SpecularRay;
    }

    RefractionRay   = normalize(lerp(RefractionRay, normalize(Normal + NextRandomUnitSphereVec3(RandomSeed)), Material.RefractionRoughness * Material.RefractionRoughness));
    OutRayDirection = lerp(DiffuseRay, SpecularRay, DoSpecular);
    OutRayDirection = lerp(OutRayDirection, RefractionRay, DoRefraction);

    if (any(isnan(OutRayDirection)) || any(isinf(OutRayDirection)) || dot(OutRayDirection, OutRayDirection) < 1e-8)
    {
        HasValidRay    = 0;
        OutRayDirection = float3(0.0, 0.0, 1.0);
        OutRayOrigin    = Input.Position;
        return;
    }

    OutRayDirection = normalize(OutRayDirection);
    OutRayOrigin    = Input.Position + OutRayDirection * RayOffset;

    SampleColor += EmissiveContribution * RayColor;

    if (DoRefraction == 0.0)
    {
        float3 Albedo = GetMaterialAlbedo(Material, Input.TexCoords);
        RayColor *= lerp(Albedo.rgb, Material.SpecularColor.rgb, DoSpecular);
    }

    RayColor /= RayProbability;

#if ENABLE_RUSSIAN_ROULETTE
    float Probability = max(RayColor.r, max(RayColor.g, RayColor.b));
    if (NextRandom(RandomSeed) > Probability)
    {
        HasValidRay = 0;
        return;
    }

    RayColor /= Probability;
#endif

    HasValidRay = 1;
}

#endif


