#include "random.hlsli"
#include "math.hlsli"
#include "primitives.hlsli"
#include "utilities.hlsli"
#include "ray.hlsli"
#include "bvh.hlsli"

#define BACKGROUND_TYPE_NONE 0
#define BACKGROUND_TYPE_GRADIENT 1
#define BACKGROUND_TYPE_SKYBOX 2

#define VIEW_MODE_RENDER 0
#define VIEW_MODE_NORMALS 1
#define VIEW_MODE_GEOMETRIC_NORMALS 2
#define VIEW_MODE_TANGENTS 3
#define VIEW_MODE_ALBEDO 4
#define VIEW_MODE_BARYCENTRICS 5
#define VIEW_MODE_TEXCOORDS 6
#define VIEW_MODE_BVH_INTERSECTION 7

#define NUM_THREADS 16
#define MAX_NUM_BOUNCES 1024
#define SIGMA 0.0001
#define RAY_OFFSET 0.001
#define GAMMA 2.2
#define SKYBOX_MULTIPLIER 1.0

#define ENABLE_QUAD_BACK_FACE_CULLING 1
#define ENABLE_RUSSIAN_ROULETTE 1

[[vk::binding(0)]] RWTexture2D<float4> uOutput;
[[vk::binding(1)]] RWTexture2D<float4> uPreviousFrame;

[[vk::combinedImageSampler]][[vk::binding(2)]]
TextureCube<float4> uSkybox : register(t2);

[[vk::combinedImageSampler]][[vk::binding(2)]]
SamplerState uSkyboxSampler : register(s2);

[[vk::binding(0, 1)]] Texture2D<float4> uTextures[];
[[vk::binding(0, 1)]] SamplerState uTexturesSampler : register(s0, space1);

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
    uint  NumQuads;
    uint  NumSpheres;
    uint  NumMeshes;
    uint  NumMaterials;
    uint  NumBvhNodes;
    uint  NumTriangles;
    uint  BackgroundType;
    uint  NumBounces;
    uint  ViewMode;
    float GradientLightStrength;
    // Padding
    uint  Padding0;
    uint  Padding1;
};

[[vk::binding(3)]]
ConstantBuffer<SCameraBuffer> uCamera;

[[vk::binding(4)]]
ConstantBuffer<SRandomBuffer> uRandom;

[[vk::binding(5)]]
ConstantBuffer<SSceneBuffer> uScene;

[[vk::binding(6)]]  StructuredBuffer<SQuad>           Quads;
[[vk::binding(7)]]  StructuredBuffer<SSphere>         Spheres;
[[vk::binding(8)]]  StructuredBuffer<SMaterial>       Materials;
[[vk::binding(9)]]  StructuredBuffer<SVertexPosition> VertexPositions;
[[vk::binding(10)]] StructuredBuffer<SVertex>         Vertices;
[[vk::binding(11)]] StructuredBuffer<uint3>           Indices;
[[vk::binding(12)]] StructuredBuffer<STriangle>       Triangles;
[[vk::binding(13)]] StructuredBuffer<SMesh>           Meshes;
[[vk::binding(14)]] StructuredBuffer<SBoundingBox>    BvhNodes;

bool IsAlmostZero(float3 Value)
{
    return Value.x <= SIGMA && Value.y <= SIGMA && Value.z <= SIGMA;
}

void HitQuad(SQuad Quad, SRay Ray, inout SRayPayLoad PayLoad)
{
    float3 Q      = Quad.Position.xyz;
    float3 U      = Quad.Edge0.xyz;
    float3 V      = Quad.Edge1.xyz;
    float3 N      = cross(U, V);
    float3 W      = N / dot(N, N);
    float3 Normal = normalize(N);
    float  D      = dot(Normal, Q);
    float  DdotN  = dot(Ray.Direction, Normal);

#if ENABLE_QUAD_BACK_FACE_CULLING
    if (DdotN > 0.0)
    {
        return;
    }
#endif

    if (abs(DdotN) < SIGMA)
    {
        return;
    }

    float t = (D - dot(Normal, Ray.Origin)) / DdotN;
    if (PayLoad.MinT < t && t < PayLoad.MaxT)
    {
        if (t < PayLoad.T)
        {
            float3 Intersection = Ray.Origin + (Ray.Direction * t);
            float3 PlanarHit    = Intersection - Q;

            float Alpha = dot(W, cross(PlanarHit, V));
            float Beta  = dot(W, cross(U, PlanarHit));

            if (Alpha < 0.0 || 1.0 < Alpha || Beta < 0.0 || 1.0 < Beta)
            {
                return;
            }

            PayLoad.T             = t;
            PayLoad.MaterialIndex = Quad.MaterialIndex;
            PayLoad.bFrontFace    = 1;
            PayLoad.bFromInside   = 0;
            PayLoad.Position      = Ray.Origin + Ray.Direction * PayLoad.T;

            if (DdotN >= 0.0)
            {
                PayLoad.Normal = -Normal;
            }
            else
            {
                PayLoad.Normal = Normal;
            }
        }
    }
}

void HitSphere(SSphere Sphere, SRay Ray, inout SRayPayLoad PayLoad)
{
    float3 SpherePos    = Sphere.PositionAndRadius.xyz;
    float  SphereRadius = Sphere.PositionAndRadius.w;

    float3 oc = Ray.Origin - SpherePos;

    float a = dot(Ray.Direction, Ray.Direction);
    float b = dot(Ray.Direction, oc);
    float c = dot(oc, oc) - (SphereRadius * SphereRadius);

    float Discriminant = (b * b) - a * c;
    if (Discriminant < 0.0)
    {
        return;
    }

    float sqrtDiscriminant = sqrt(Discriminant);
    float t = (-b - sqrtDiscriminant) / a;

    uint bFromInside = 0;
    if (t <= PayLoad.MinT || t >= PayLoad.MaxT)
    {
        t = (-b + sqrtDiscriminant) / a;
        bFromInside = 1;

        if (t <= PayLoad.MinT || t >= PayLoad.MaxT)
        {
            return;
        }
    }

    if (t <= PayLoad.T)
    {
        PayLoad.T             = t;
        PayLoad.MaterialIndex = Sphere.MaterialIndex;
        PayLoad.Position      = Ray.Origin + Ray.Direction * PayLoad.T;
        PayLoad.bFromInside   = bFromInside;
        PayLoad.bFrontFace    = 1 - bFromInside;
        PayLoad.Normal        = normalize((PayLoad.Position - SpherePos) / SphereRadius) * (bFromInside ? -1.0 : 1.0);
    }
}

SHitInfo HitTriangle(float3 Vertex0, float3 Vertex1, float3 Vertex2, float3 RayOrigin, float3 RayDirection)
{
    SHitInfo HitInfo;
    HitInfo.Dist = LARGE_NUMBER;

    float3 Edge1 = Vertex1 - Vertex0;
    float3 Edge2 = Vertex2 - Vertex0;
    float3 DirectionCrossEdge2 = cross(RayDirection, Edge2);

    float Determinant = dot(Edge1, DirectionCrossEdge2);
    if (abs(Determinant) < SIGMA)
    {
        return HitInfo;
    }

    float3 RayOriginToVertex0 = RayOrigin - Vertex0;

    float RecipDeterminant = 1.0 / Determinant;
    float U = RecipDeterminant * dot(RayOriginToVertex0, DirectionCrossEdge2);
    if (U < 0.0 || U > 1.0)
    {
        return HitInfo;
    }

    float3 RayOriginToVertex0CrossEdge1 = cross(RayOriginToVertex0, Edge1);

    float V = RecipDeterminant * dot(RayDirection, RayOriginToVertex0CrossEdge1);
    if (V < 0.0 || U + V > 1.0)
    {
        return HitInfo;
    }

    HitInfo.Dist = RecipDeterminant * dot(Edge2, RayOriginToVertex0CrossEdge1);
    HitInfo.BaryCentrics = float2(U, V);
    return HitInfo;
}

void HitMesh(uint RootBoxIndex, SRay Ray, inout SRayPayLoad PayLoad, inout int2 Stats)
{
    const uint MaxDepth = BVH_MAX_DEPTH;
    uint Stack[MaxDepth];

    int StackIndex = 0;
    Stack[StackIndex] = RootBoxIndex;

    SHitInfo LastHitInfo;

    int LastTriangleHitIndex = -1;
    while (StackIndex >= 0)
    {
        uint NodeIndex = Stack[StackIndex];
        StackIndex--;

        SBoundingBox Node = BvhNodes[NodeIndex];
        if (asuint(Node.BoxMaxAndNumTriangles.w) > 0)
        {
            uint LastTriangleIndex = asuint(Node.BoxMinAndIndex.w) + asuint(Node.BoxMaxAndNumTriangles.w);
            for (uint TriangleIndex = asuint(Node.BoxMinAndIndex.w); TriangleIndex < LastTriangleIndex; TriangleIndex++)
            {
                uint3  Indicies  = Indices[TriangleIndex];
                float3 Position0 = VertexPositions[Indicies.x].Position.xyz;
                float3 Position1 = VertexPositions[Indicies.y].Position.xyz;
                float3 Position2 = VertexPositions[Indicies.z].Position.xyz;

                SHitInfo HitInfo = HitTriangle(Position0, Position1, Position2, Ray.Origin, Ray.Direction);
                Stats.y++;

                if (HitInfo.Dist > PayLoad.MinT && HitInfo.Dist < PayLoad.MaxT && HitInfo.Dist < PayLoad.T)
                {
                    STriangle Triangle = Triangles[TriangleIndex];
                    uint MaterialIndex = min((uint)Triangle.MaterialIndex, uScene.NumMaterials - 1);
                    SMaterial Material = Materials[MaterialIndex];

                    if (Material.AlphaMaskTexIndex != INVALID_BINDLESS_ID)
                    {
                        float2 TexCoords0 = Vertices[Indicies.x].TexCoord.xy;
                        float2 TexCoords1 = Vertices[Indicies.y].TexCoord.xy;
                        float2 TexCoords2 = Vertices[Indicies.z].TexCoord.xy;

                        float3 Barycentrics = float3(HitInfo.BaryCentrics, 1.0 - (HitInfo.BaryCentrics.x + HitInfo.BaryCentrics.y));
                        float2 TexCoords = (Barycentrics.x * TexCoords1) + (Barycentrics.y * TexCoords2) + (Barycentrics.z * TexCoords0);
                        float Alpha = uTextures[Material.AlphaMaskTexIndex].SampleLevel(uTexturesSampler, TexCoords, 0.0).r;
                        if (Alpha < 0.9)
                        {
                            continue;
                        }
                    }

                    PayLoad.T            = HitInfo.Dist;
                    LastTriangleHitIndex = (int)TriangleIndex;
                    LastHitInfo          = HitInfo;
                }
            }
        }
        else
        {
            uint ChildIndex1 = asuint(Node.BoxMinAndIndex.w);
            uint ChildIndex2 = asuint(Node.BoxMinAndIndex.w) + 1;

            float Dist1 = IntersectRayAABB(BvhNodes[ChildIndex1].BoxMinAndIndex.xyz, BvhNodes[ChildIndex1].BoxMaxAndNumTriangles.xyz, Ray.Origin, Ray.InvDirection);
            float Dist2 = IntersectRayAABB(BvhNodes[ChildIndex2].BoxMinAndIndex.xyz, BvhNodes[ChildIndex2].BoxMaxAndNumTriangles.xyz, Ray.Origin, Ray.InvDirection);
            Stats.x += 2;

            if (Dist1 > Dist2)
            {
                if (Dist1 < PayLoad.T)
                {
                    Stack[++StackIndex] = ChildIndex1;
                }
                
                if (Dist2 < PayLoad.T)
                {
                    Stack[++StackIndex] = ChildIndex2;
                }
            }
            else
            {
                if (Dist2 < PayLoad.T)
                {
                    Stack[++StackIndex] = ChildIndex2;
                }

                if (Dist1 < PayLoad.T)
                {
                    Stack[++StackIndex] = ChildIndex1;
                }
            }
        }
    }

    if (LastTriangleHitIndex >= 0)
    {
        const uint3 Indicies = Indices[LastTriangleHitIndex];

        float2 TexCoords0 = Vertices[Indicies.x].TexCoord.xy;
        float2 TexCoords1 = Vertices[Indicies.y].TexCoord.xy;
        float2 TexCoords2 = Vertices[Indicies.z].TexCoord.xy;

        float3 Normal0    = Vertices[Indicies.x].Normal.xyz;
        float3 Normal1    = Vertices[Indicies.y].Normal.xyz;
        float3 Normal2    = Vertices[Indicies.z].Normal.xyz;

        float3 Tangent0   = Vertices[Indicies.x].Tangent.xyz;
        float3 Tangent1   = Vertices[Indicies.y].Tangent.xyz;
        float3 Tangent2   = Vertices[Indicies.z].Tangent.xyz;

        STriangle Triangle = Triangles[LastTriangleHitIndex];
        PayLoad.BaryCentrics  = float3(LastHitInfo.BaryCentrics, 1.0 - (LastHitInfo.BaryCentrics.x + LastHitInfo.BaryCentrics.y));
        PayLoad.Normal        = normalize((PayLoad.BaryCentrics.x * Normal1)  + (PayLoad.BaryCentrics.y * Normal2)  + (PayLoad.BaryCentrics.z * Normal0));
        PayLoad.Tangent       = normalize((PayLoad.BaryCentrics.x * Tangent1) + (PayLoad.BaryCentrics.y * Tangent2) + (PayLoad.BaryCentrics.z * Tangent0));
        PayLoad.TexCoords     = (PayLoad.BaryCentrics.x * TexCoords1) + (PayLoad.BaryCentrics.y * TexCoords2) + (PayLoad.BaryCentrics.z * TexCoords0);
        PayLoad.MaterialIndex = Triangle.MaterialIndex;
        PayLoad.Position      = Ray.Origin + PayLoad.T * Ray.Direction;
        PayLoad.bFromInside   = false;

        float DdotN = dot(Ray.Direction, PayLoad.Normal);
        if (DdotN < 0.0)
        {
            PayLoad.bFrontFace = true;
        }
        else
        {
            PayLoad.bFrontFace = false;
        }
    }
}

bool TraceRay(SRay Ray, inout SRayPayLoad PayLoad, inout int2 Stats)
{
    for (uint i = 0; i < uScene.NumSpheres; i++)
    {
        SSphere Sphere = Spheres[i];
        HitSphere(Sphere, Ray, PayLoad);
    }

    for (uint i = 0; i < uScene.NumQuads; i++)
    {
        SQuad Quad = Quads[i];
        HitQuad(Quad, Ray, PayLoad);
    }

    for (uint i = 0; i < uScene.NumMeshes; i++)
    {
        SMesh Mesh = Meshes[i];
        HitMesh(Mesh.BoundingBoxIndex, Ray, PayLoad, Stats);
    }

    return PayLoad.T < PayLoad.MaxT;
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

float3 CalculateFilmTarget(int2 Pixel, int2 Size, float2 Jitter)
{
    float3 CameraPosition = uCamera.Position.xyz;
    float3 CamForward     = normalize(uCamera.Forward.xyz);

    float3 CamUp = float3(0.0, 1.0, 0.0);
    CamUp = normalize(CamUp - dot(CamUp, CamForward) * CamForward);

    float3 CamRight = normalize(cross(CamUp, CamForward));

    float  AspectRatio  = (float)Size.x / (float)Size.y;
    float  FieldOfView  = clamp(uCamera.FieldOfViewDegrees, 30.0, 120.0);
    float  FilmDistance = 1.0 / tan(FieldOfView * 0.5 * PI / 180.0);
    float3 FilmCenter   = CameraPosition + (CamForward * FilmDistance);

    float2 FilmUV = (float2(Pixel) + Jitter) / float2(Size.xy);
    FilmUV.y = 1.0 - FilmUV.y;
    FilmUV = FilmUV * 2.0;

    float2 FilmCorner = float2(-1.0, -1.0);
    float2 FilmCoord  = FilmCorner + FilmUV;
    FilmCoord.x = FilmCoord.x * AspectRatio;

    return FilmCenter + (CamRight * FilmCoord.x) + (CamUp * FilmCoord.y);
}

float3 GetEnvironmentLight(float3 RayDirection)
{
    if (uScene.BackgroundType == BACKGROUND_TYPE_NONE)
    {
        return float3(0.0, 0.0, 0.0);
    }
    else if (uScene.BackgroundType == BACKGROUND_TYPE_GRADIENT)
    {
        float3 UnitDir  = normalize(RayDirection);
        float  Alpha    = 0.5 * (UnitDir.y + 1.0);
        float3 Color    = (1.0 - Alpha) * float3(1.0, 1.0, 1.0) + Alpha * float3(0.5, 0.7, 1.0);
        float  Strength = max(1.0, uScene.GradientLightStrength);
        return Color * Strength;
    }
    else if (uScene.BackgroundType == BACKGROUND_TYPE_SKYBOX)
    {
        float3 UnitDirection = normalize(RayDirection);
        float4 SkyboxColor   = uSkybox.SampleLevel(uSkyboxSampler, UnitDirection, 0.0);
        return SkyboxColor.rgb * SKYBOX_MULTIPLIER;
    }
    else
    {
        return float3(0.0, 0.0, 0.0);
    }
}

float3 GetColorForRay(SRay Ray, inout uint RandomSeed)
{
    float3 RayColor    = float3(1.0, 1.0, 1.0);
    float3 SampleColor = float3(0.0, 0.0, 0.0);

    int2 Stats = int2(0, 0);

    const uint MaxBounces = min(uScene.NumBounces, MAX_NUM_BOUNCES) + 1;
    for (uint i = 0; i < MaxBounces; i++)
    {
        SRayPayLoad PayLoad;
        PayLoad.MinT        = 0.0001;
        PayLoad.MaxT        = 100000.0;
        PayLoad.T           = PayLoad.MaxT;
        PayLoad.bFrontFace  = 0;
        PayLoad.bFromInside = 0;

        if (TraceRay(Ray, PayLoad, Stats))
        {
            const uint MaterialIndex = min(PayLoad.MaterialIndex, uScene.NumMaterials - 1);
            SMaterial Material = Materials[MaterialIndex];

            float3 Normal;
            if (Material.NormalTexIndex != INVALID_BINDLESS_ID)
            {
                float3 NormalMap = uTextures[Material.NormalTexIndex].SampleLevel(uTexturesSampler, PayLoad.TexCoords, 0.0).rgb;
                NormalMap = normalize(NormalMap * 2.0 - 1.0);
                
                const float3 BiTangent = cross(PayLoad.Normal, PayLoad.Tangent);
                
                const float3x3 TBNMatrix = float3x3(PayLoad.Tangent, BiTangent, PayLoad.Normal);
                Normal = normalize(mul(NormalMap, TBNMatrix));

                if (any(isnan(Normal)) || any(isinf(Normal)))
                {
                    Normal = PayLoad.Normal;
                }
            }
            else
            {
                Normal = PayLoad.Normal;
            }

            if (PayLoad.bFromInside)
            {
                RayColor *= exp(-Material.AbsorbtionColor.rgb * PayLoad.T);
            }

            float SpecularRoughness;
            if (Material.RoughnessTexIndex != INVALID_BINDLESS_ID)
            {
                SpecularRoughness = uTextures[Material.RoughnessTexIndex].SampleLevel(uTexturesSampler, PayLoad.TexCoords, 0.0).r;
            }
            else
            {
                SpecularRoughness = Material.SpecularRoughness;
            }

            float SpecularChance;
            if (Material.MetallicTexIndex != INVALID_BINDLESS_ID)
            {
                SpecularChance = uTextures[Material.MetallicTexIndex].SampleLevel(uTexturesSampler, PayLoad.TexCoords, 0.0).r;
            }
            else
            {
                SpecularChance = Material.SpecularChance;
            }

            float RefractionChance = Material.RefractionChance;
            if ((SpecularChance > 0.0) || (RefractionChance > 0.0))
            {
                float IncidenceOfRefraction1 = PayLoad.bFromInside ? Material.IncidenceOfRefraction : 1.0;
                float IncidenceOfRefraction2 = PayLoad.bFromInside ? 1.0 : Material.IncidenceOfRefraction;
                SpecularChance = FresnelReflectAmount(IncidenceOfRefraction1, IncidenceOfRefraction2, Ray.Direction, Normal, Material.SpecularChance, 1.0);

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
                DoSpecular = 1.0;
                RayProbability = SpecularChance;
            }
            else if (RefractionChance > 0.0 && RaySelectRoll < (SpecularChance + RefractionChance))
            {
                DoRefraction = 1.0;
                RayProbability = RefractionChance;
            }
            else
            {
                RayProbability = 1.0 - (SpecularChance + RefractionChance);
            }

            RayProbability = max(RayProbability, 0.001);

            float3 RayDirection = Ray.Direction;
            float3 DiffuseRay   = normalize(Normal + NextRandomUnitSphereVec3(RandomSeed));
            float3 SpecularRay  = reflect(RayDirection, Normal);
            SpecularRay = normalize(lerp(SpecularRay, DiffuseRay, SpecularRoughness * SpecularRoughness));

            float3 RefractionRay = refract(RayDirection, Normal, PayLoad.bFromInside ? Material.IncidenceOfRefraction : (1.0 / Material.IncidenceOfRefraction));
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
            float3 RayPosition = PayLoad.Position + RayDirection * RAY_OFFSET;

            SampleColor += Material.EmissiveColor.rgb * RayColor;

            if (DoRefraction == 0.0)
            {
                float3 Albedo;
                if (Material.AlbedoTexIndex != INVALID_BINDLESS_ID)
                {
                    Albedo = uTextures[Material.AlbedoTexIndex].SampleLevel(uTexturesSampler, PayLoad.TexCoords, 0.0).rgb;
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

            Ray.Origin       = RayPosition;
            Ray.Direction    = RayDirection;
            Ray.InvDirection = 1.0 / Ray.Direction;
        }
        else
        {
            SampleColor += GetEnvironmentLight(Ray.Direction) * RayColor;
            break;
        }
    }

    return SampleColor;
}

float3 GetNormalForRay(SRay Ray)
{
    SRayPayLoad PayLoad;
    PayLoad.MinT        = 0.0001;
    PayLoad.MaxT        = 100000.0;
    PayLoad.T           = PayLoad.MaxT;
    PayLoad.bFrontFace  = 0;
    PayLoad.bFromInside = 0;

    int2 Stats = int2(0, 0);
    if (TraceRay(Ray, PayLoad, Stats))
    {
        const uint MaterialIndex = min(PayLoad.MaterialIndex, uScene.NumMaterials - 1);

        float3 Normal;
        SMaterial Material = Materials[MaterialIndex];
        if (Material.NormalTexIndex != INVALID_BINDLESS_ID)
        {
            float3 NormalMap = uTextures[Material.NormalTexIndex].SampleLevel(uTexturesSampler, PayLoad.TexCoords, 0.0).rgb;
            NormalMap = normalize(NormalMap * 2.0 - 1.0);

            float3 BiTangent = cross(PayLoad.Normal, PayLoad.Tangent);
            
            float3x3 TBN = float3x3(PayLoad.Tangent, BiTangent, PayLoad.Normal);
            Normal = normalize(mul(NormalMap, TBN));

            if (any(isnan(Normal)) || any(isinf(Normal)))
            {
                Normal = PayLoad.Normal;
            }
        }
        else
        {
            Normal = PayLoad.Normal;
        }

        return (Normal + float3(1.0, 1.0, 1.0)) * 0.5;
    }
    else
    {
        return float3(0.0, 0.0, 0.0);
    }
}

float3 GetGeometricNormalForRay(SRay Ray)
{
    SRayPayLoad PayLoad;
    PayLoad.MinT        = 0.0001;
    PayLoad.MaxT        = 100000.0;
    PayLoad.T           = PayLoad.MaxT;
    PayLoad.bFrontFace  = 0;
    PayLoad.bFromInside = 0;

    int2 Stats = int2(0, 0);
    if (TraceRay(Ray, PayLoad, Stats))
    {
        return (normalize(PayLoad.Normal) + float3(1.0, 1.0, 1.0)) * 0.5;
    }
    else
    {
        return float3(0.0, 0.0, 0.0);
    }
}

float3 GetTangentForRay(SRay Ray)
{
    SRayPayLoad PayLoad;
    PayLoad.MinT        = 0.0001;
    PayLoad.MaxT        = 100000.0;
    PayLoad.T           = PayLoad.MaxT;
    PayLoad.bFrontFace  = 0;
    PayLoad.bFromInside = 0;

    int2 Stats = int2(0, 0);
    if (TraceRay(Ray, PayLoad, Stats))
    {
        return PayLoad.Tangent;
    }
    else
    {
        return float3(0.0, 0.0, 0.0);
    }
}

float3 GetBarycentricsForRay(SRay Ray)
{
    SRayPayLoad PayLoad;
    PayLoad.MinT         = 0.0001;
    PayLoad.MaxT         = 100000.0;
    PayLoad.T            = PayLoad.MaxT;
    PayLoad.bFrontFace   = 0;
    PayLoad.bFromInside  = 0;
    PayLoad.BaryCentrics = float3(0.0, 0.0, 0.0);

    int2 Stats = int2(0, 0);
    if (TraceRay(Ray, PayLoad, Stats))
    {
        return PayLoad.BaryCentrics;
    }
    else
    {
        return float3(0.0, 0.0, 0.0);
    }
}

float3 GetTexCoordsForRay(SRay Ray)
{
    SRayPayLoad PayLoad;
    PayLoad.MinT        = 0.0001;
    PayLoad.MaxT        = 100000.0;
    PayLoad.T           = PayLoad.MaxT;
    PayLoad.bFrontFace  = 0;
    PayLoad.bFromInside = 0;
    PayLoad.TexCoords   = float2(0.0, 0.0);

    int2 Stats = int2(0, 0);
    if (TraceRay(Ray, PayLoad, Stats))
    {
        return float3(PayLoad.TexCoords, 0.0);
    }
    else
    {
        return float3(0.0, 0.0, 0.0);
    }
}

float3 GetAlbedoForRay(SRay Ray)
{
    SRayPayLoad PayLoad;
    PayLoad.MinT        = 0.0001;
    PayLoad.MaxT        = 100000.0;
    PayLoad.T           = PayLoad.MaxT;
    PayLoad.bFrontFace  = 0;
    PayLoad.bFromInside = 0;

    int2 Stats = int2(0, 0);
    if (TraceRay(Ray, PayLoad, Stats))
    {
        const uint MaterialIndex = min(PayLoad.MaterialIndex, uScene.NumMaterials - 1);

        SMaterial Material = Materials[MaterialIndex];
        if (Material.AlbedoTexIndex != INVALID_BINDLESS_ID)
        {
            return uTextures[Material.AlbedoTexIndex].SampleLevel(uTexturesSampler, PayLoad.TexCoords, 0.0).rgb;
        }
        else
        {
            return Material.AlbedoColor.rgb;
        }
    }
    else
    {
        return float3(0.0, 0.0, 0.0);
    }
}

float3 GetColorForRay_BvhDebug(SRay Ray)
{
    SRayPayLoad PayLoad;
    PayLoad.MinT        = 0.0001;
    PayLoad.MaxT        = 100000.0;
    PayLoad.T           = PayLoad.MaxT;
    PayLoad.bFrontFace  = 0;
    PayLoad.bFromInside = 0;

    int2 Stats = int2(0, 0);
    TraceRay(Ray, PayLoad, Stats);

    float3 BoxTestColor      = float3((float)Stats.x, (float)Stats.x, (float)Stats.x) / 100.0;
    float3 TriangleTestColor = float3((float)Stats.y, (float)Stats.y, (float)Stats.y) / 100.0;
    return TriangleTestColor;
}

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    const int2 Pixel = (int2)DispatchThreadId.xy;

    uint2 Dim;
    uOutput.GetDimensions(Dim.x, Dim.y);
    
    const int2 ActualSize = (int2)Dim;

    uint RandomSeed = InitRandom(DispatchThreadId.xy, (uint)ActualSize.x, uRandom.FrameIndex);

    float2 Jitter = float2(NextRandom(RandomSeed), NextRandom(RandomSeed)) - 0.5;

    const float3 CameraPosition = uCamera.Position.xyz;
    const float3 FilmTarget     = CalculateFilmTarget(Pixel, ActualSize, Jitter);

    SRay Ray;
    Ray.Origin       = CameraPosition;
    Ray.Direction    = normalize(FilmTarget - CameraPosition);
    Ray.InvDirection = 1.0 / Ray.Direction;

    if (uScene.ViewMode == VIEW_MODE_RENDER)
    {
        float3 SampleColor   = GetColorForRay(Ray, RandomSeed);
        float4 PreviousColor = uPreviousFrame[Pixel];
        float3 CurrentColor  = lerp(PreviousColor.rgb, SampleColor, 1.0 / (float)(uRandom.FrameIndex + 1));
        uOutput[Pixel] = float4(CurrentColor, 1.0);
    }
    else if (uScene.ViewMode == VIEW_MODE_NORMALS)
    {
        float3 HitNormal = GetNormalForRay(Ray);
        uOutput[Pixel] = float4(HitNormal, 1.0);
    }
    else if (uScene.ViewMode == VIEW_MODE_GEOMETRIC_NORMALS)
    {
        float3 HitNormal = GetGeometricNormalForRay(Ray);
        uOutput[Pixel] = float4(HitNormal, 1.0);
    }
    else if (uScene.ViewMode == VIEW_MODE_TANGENTS)
    {
        float3 HitTangent = GetTangentForRay(Ray);
        uOutput[Pixel] = float4(HitTangent, 1.0);
    }
    else if (uScene.ViewMode == VIEW_MODE_ALBEDO)
    {
        float3 HitAlbedo = GetAlbedoForRay(Ray);
        uOutput[Pixel] = float4(HitAlbedo, 1.0);
    }
    else if (uScene.ViewMode == VIEW_MODE_BARYCENTRICS)
    {
        float3 HitBarycentrics = GetBarycentricsForRay(Ray);
        uOutput[Pixel] = float4(HitBarycentrics, 1.0);
    }
    else if (uScene.ViewMode == VIEW_MODE_TEXCOORDS)
    {
        float3 HitTexCoords = GetTexCoordsForRay(Ray);
        uOutput[Pixel] = float4(HitTexCoords, 1.0);
    }
    else if (uScene.ViewMode == VIEW_MODE_BVH_INTERSECTION)
    {
        float3 Color = GetColorForRay_BvhDebug(Ray);
        uOutput[Pixel] = float4(Color, 1.0);
    }
}
