#include "random.hlsli"
#include "math.hlsli"
#include "primitives.hlsli"
#include "utilities.hlsli"
#include "ray.hlsli"
#include "bvh.hlsli"
#define COMMON_STRUCTS_NO_HW_RAYTRACE
#include "common_structs.hlsli"

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
#define ENABLE_SOFTWARE_TRACING 1

[[vk::binding(2)]] RWTexture2D<float4> OutputTexture;
[[vk::binding(3)]] RWTexture2D<float4> PreviousFrameTexture;

[[vk::combinedImageSampler]][[vk::binding(4)]]
TextureCube<float4> SkyboxTexture : register(t2);

[[vk::combinedImageSampler]][[vk::binding(4)]]
SamplerState SkyboxSampler : register(s2);

[[vk::binding(0)]] Texture2D<float4> Textures[];
[[vk::binding(0)]] SamplerState TexturesSampler : register(s0);

struct SSceneBuffer
{
    uint  NumQuads;
    uint  NumSpheres;
    uint  NumMeshes;
    uint  NumMaterials;
    uint  NumBvhNodes;
    uint  NumTlasNodes;
    uint  NumTriangles;
    uint  BackgroundType;
    uint  NumBounces;
    uint  ViewMode;
    float GradientLightStrength;
    // Padding
    uint  Padding0;
    uint  Padding1;
    uint  Padding2;
};

[[vk::binding(14)]]
ConstantBuffer<SCameraBuffer> Camera;

[[vk::binding(15)]]
ConstantBuffer<SRandomBuffer> Random;

[[vk::binding(16)]]
ConstantBuffer<SSceneBuffer> Scene;

[[vk::binding(5)]]  StructuredBuffer<SQuad>           Quads;
[[vk::binding(6)]]  StructuredBuffer<SSphere>         Spheres;
[[vk::binding(7)]]  StructuredBuffer<SMaterial>       Materials;
[[vk::binding(8)]]  StructuredBuffer<SVertexPosition> VertexPositions;
[[vk::binding(9)]]  StructuredBuffer<SVertex>         Vertices;
[[vk::binding(10)]] StructuredBuffer<uint3>           Indices;
[[vk::binding(11)]] StructuredBuffer<STriangle>       Triangles;
[[vk::binding(12)]] StructuredBuffer<SMesh>           Meshes;
[[vk::binding(13)]] StructuredBuffer<SBoundingBox>    BvhNodes;
[[vk::binding(17)]] StructuredBuffer<SBoundingBox>    TlasNodes;

#include "shading.hlsli"
#include "trace_common.hlsli"

bool IsAlmostZero(float3 Value)
{
    return Value.x <= SIGMA && Value.y <= SIGMA && Value.z <= SIGMA;
}

void HitQuad(SQuad Quad, SRay Ray, inout SRayPayload Payload)
{
    float3 Q      = Quad.Position.xyz;
    float3 U      = Quad.Edge0.xyz;
    float3 V      = Quad.Edge1.xyz;
    float3 N      = cross(U, V);
    float3 W      = N / dot(N, N);
    float3 Normal = SafeNormalize(N, float3(0.0, 1.0, 0.0));
    float  D      = dot(Normal, Q);
    float  DDotN  = dot(Ray.Direction, Normal);

#if ENABLE_QUAD_BACK_FACE_CULLING
    if (DDotN > 0.0)
    {
        return;
    }
#endif

    if (abs(DDotN) < SIGMA)
    {
        return;
    }

    float t = (D - dot(Normal, Ray.Origin)) / DDotN;
    if (Payload.MinT < t && t < Payload.MaxT)
    {
        if (t < Payload.T)
        {
            float3 Intersection = Ray.Origin + (Ray.Direction * t);
            float3 PlanarHit    = Intersection - Q;

            float Alpha = dot(W, cross(PlanarHit, V));
            float Beta  = dot(W, cross(U, PlanarHit));

            if (Alpha < 0.0 || 1.0 < Alpha || Beta < 0.0 || 1.0 < Beta)
            {
                return;
            }

            Payload.T             = t;
            Payload.MaterialIndex = Quad.MaterialIndex;
            Payload.FrontFace    = 1;
            Payload.FromInside   = 0;
            Payload.Position      = Ray.Origin + Ray.Direction * Payload.T;
            Payload.Barycentrics  = float3(Alpha, Beta, saturate(1.0 - Alpha - Beta));
            Payload.TexCoords     = float2(Alpha, Beta);
            Payload.Tangent       = SafeNormalize(U, float3(1.0, 0.0, 0.0));

            if (DDotN >= 0.0)
            {
                Payload.Normal = -Normal;
            }
            else
            {
                Payload.Normal = Normal;
            }
        }
    }
}

void HitSphere(SSphere Sphere, SRay Ray, inout SRayPayload Payload)
{
    float3 SpherePos    = Sphere.PositionAndRadius.xyz;
    float  SphereRadius = Sphere.PositionAndRadius.w;

    float3 Oc = Ray.Origin - SpherePos;

    float a = dot(Ray.Direction, Ray.Direction);
    float b = dot(Ray.Direction, Oc);
    float c = dot(Oc, Oc) - (SphereRadius * SphereRadius);

    float Discriminant = (b * b) - a * c;
    if (Discriminant < 0.0)
    {
        return;
    }

    float SqrtDiscriminant = sqrt(Discriminant);
    float t = (-b - SqrtDiscriminant) / a;

    uint FromInside = 0;
    if (t <= Payload.MinT || t >= Payload.MaxT)
    {
        t = (-b + SqrtDiscriminant) / a;
        FromInside = 1;

        if (t <= Payload.MinT || t >= Payload.MaxT)
        {
            return;
        }
    }

    if (t <= Payload.T)
    {
        Payload.T             = t;
        Payload.MaterialIndex = Sphere.MaterialIndex;
        Payload.Position      = Ray.Origin + Ray.Direction * Payload.T;
        Payload.FromInside   = FromInside;
        Payload.FrontFace    = 1 - FromInside;

        const float3 SphereNormal = SafeNormalize((Payload.Position - SpherePos) / SphereRadius, float3(0.0, 1.0, 0.0));
        Payload.Normal = SphereNormal * (FromInside ? -1.0 : 1.0);

        const float3 UpVector = (abs(Payload.Normal.y) > 0.999) ? float3(1.0, 0.0, 0.0) : float3(0.0, 1.0, 0.0);
        Payload.Tangent = SafeNormalize(cross(UpVector, Payload.Normal), float3(1.0, 0.0, 0.0));

        const float SphereU = 0.5 + atan2(SphereNormal.z, SphereNormal.x) / (2.0 * PI);
        const float SphereV = 0.5 - asin(clamp(SphereNormal.y, -1.0, 1.0)) / PI;
        Payload.TexCoords = float2(SphereU, SphereV);
        Payload.Barycentrics = float3(0.0, 0.0, 0.0);
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
    float  RecipDeterminant   = 1.0 / Determinant;

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

    HitInfo.Dist         = RecipDeterminant * dot(Edge2, RayOriginToVertex0CrossEdge1);
    HitInfo.Barycentrics = float2(U, V);

    return HitInfo;
}

void HitMesh(SMesh Mesh, SRay Ray, inout SRayPayload Payload, inout int2 Stats)
{
    SRay LocalRay;
    LocalRay.Origin       = mul(Mesh.WorldToLocal, float4(Ray.Origin, 1.0)).xyz;
    LocalRay.Direction    = mul((float3x3)Mesh.WorldToLocal, Ray.Direction);
    LocalRay.InvDirection = 1.0 / LocalRay.Direction;

    SRayPayload LocalPayload = Payload;

    const uint RootBoxIndex = Mesh.BoundingBoxIndex;
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
                uint3  VertexIndices  = Indices[TriangleIndex];
                float3 Position0 = VertexPositions[VertexIndices.x].Position.xyz;
                float3 Position1 = VertexPositions[VertexIndices.y].Position.xyz;
                float3 Position2 = VertexPositions[VertexIndices.z].Position.xyz;

                SHitInfo HitInfo = HitTriangle(Position0, Position1, Position2, LocalRay.Origin, LocalRay.Direction);
                Stats.y++;

                if (HitInfo.Dist > LocalPayload.MinT && HitInfo.Dist < LocalPayload.MaxT && HitInfo.Dist < LocalPayload.T)
                {
                    STriangle Triangle = Triangles[TriangleIndex];
                    if (Scene.NumMaterials == 0)
                    {
                        continue;
                    }

                    uint MaterialIndex = 0;
                    if (Triangle.MaterialIndex >= 0)
                    {
                        MaterialIndex = min((uint)Triangle.MaterialIndex, Scene.NumMaterials - 1);
                    }

                    SMaterial Material = Materials[MaterialIndex];

                    if (Material.AlphaMaskTexIndex != INVALID_BINDLESS_ID)
                    {
                        float2 TexCoords0 = Vertices[VertexIndices.x].TexCoord.xy;
                        float2 TexCoords1 = Vertices[VertexIndices.y].TexCoord.xy;
                        float2 TexCoords2 = Vertices[VertexIndices.z].TexCoord.xy;

                        const float3 Barycentrics = float3(
                            1.0 - (HitInfo.Barycentrics.x + HitInfo.Barycentrics.y),
                            HitInfo.Barycentrics.x,
                            HitInfo.Barycentrics.y);
                        float2 TexCoords = (Barycentrics.x * TexCoords0) + (Barycentrics.y * TexCoords1) + (Barycentrics.z * TexCoords2);
                        
                        float Alpha = Textures[Material.AlphaMaskTexIndex].SampleLevel(TexturesSampler, TexCoords, 0.0).r;
                        if (Alpha < 0.9)
                        {
                            continue;
                        }
                    }

                    LocalPayload.T       = HitInfo.Dist;
                    LastTriangleHitIndex = (int)TriangleIndex;
                    LastHitInfo          = HitInfo;
                }
            }
        }
        else
        {
            uint ChildIndex1 = asuint(Node.BoxMinAndIndex.w);
            uint ChildIndex2 = asuint(Node.BoxMinAndIndex.w) + 1;

            float Near1, Far1, Near2, Far2;
            IntersectRayAABBWithFar(BvhNodes[ChildIndex1].BoxMinAndIndex.xyz, BvhNodes[ChildIndex1].BoxMaxAndNumTriangles.xyz, LocalRay.Origin, LocalRay.InvDirection, Near1, Far1);
            IntersectRayAABBWithFar(BvhNodes[ChildIndex2].BoxMinAndIndex.xyz, BvhNodes[ChildIndex2].BoxMaxAndNumTriangles.xyz, LocalRay.Origin, LocalRay.InvDirection, Near2, Far2);
            Stats.x += 2;

            const float MinT = LocalPayload.MinT;
            const bool Hit1 = Near1 < LocalPayload.T && Far1 >= MinT;
            const bool Hit2 = Near2 < LocalPayload.T && Far2 >= MinT;

            if (Near1 > Near2)
            {
                if (Hit1) Stack[++StackIndex] = ChildIndex1;
                if (Hit2) Stack[++StackIndex] = ChildIndex2;
            }
            else
            {
                if (Hit2) Stack[++StackIndex] = ChildIndex2;
                if (Hit1) Stack[++StackIndex] = ChildIndex1;
            }
        }
    }

    if (LastTriangleHitIndex >= 0)
    {
        const uint3 VertexIndices = Indices[LastTriangleHitIndex];

        float2 TexCoords0 = Vertices[VertexIndices.x].TexCoord.xy;
        float2 TexCoords1 = Vertices[VertexIndices.y].TexCoord.xy;
        float2 TexCoords2 = Vertices[VertexIndices.z].TexCoord.xy;

        float3 Normal0    = Vertices[VertexIndices.x].Normal.xyz;
        float3 Normal1    = Vertices[VertexIndices.y].Normal.xyz;
        float3 Normal2    = Vertices[VertexIndices.z].Normal.xyz;

        float3 Tangent0   = Vertices[VertexIndices.x].Tangent.xyz;
        float3 Tangent1   = Vertices[VertexIndices.y].Tangent.xyz;
        float3 Tangent2   = Vertices[VertexIndices.z].Tangent.xyz;

        STriangle Triangle = Triangles[LastTriangleHitIndex];
        Payload.Barycentrics = float3(
            1.0 - (LastHitInfo.Barycentrics.x + LastHitInfo.Barycentrics.y),
            LastHitInfo.Barycentrics.x,
            LastHitInfo.Barycentrics.y);

        float3 LocalNormal  = SafeNormalize((Payload.Barycentrics.x * Normal0) + (Payload.Barycentrics.y * Normal1) + (Payload.Barycentrics.z * Normal2), float3(0.0, 1.0, 0.0));
        float3 LocalTangent = SafeNormalize((Payload.Barycentrics.x * Tangent0) + (Payload.Barycentrics.y * Tangent1) + (Payload.Barycentrics.z * Tangent2), float3(1.0, 0.0, 0.0));

        Payload.Normal        = SafeNormalize(mul(transpose((float3x3)Mesh.WorldToLocal), LocalNormal), LocalNormal);
        Payload.Tangent       = SafeNormalize(mul((float3x3)Mesh.LocalToWorld, LocalTangent), LocalTangent);
        Payload.TexCoords     = (Payload.Barycentrics.x * TexCoords0) + (Payload.Barycentrics.y * TexCoords1) + (Payload.Barycentrics.z * TexCoords2);
        if (Scene.NumMaterials > 0)
        {
            Payload.MaterialIndex = (Triangle.MaterialIndex >= 0) ? min((uint)Triangle.MaterialIndex, Scene.NumMaterials - 1) : 0;
        }
        else
        {
            Payload.MaterialIndex = 0;
        }
        Payload.T             = LocalPayload.T;
        Payload.Position      = Ray.Origin + Payload.T * Ray.Direction;
        Payload.FromInside   = false;

        float DDotN = dot(Ray.Direction, Payload.Normal);
        if (DDotN < 0.0)
        {
            Payload.FrontFace = true;
        }
        else
        {
            Payload.FrontFace = false;
        }
    }
}

void TraceTLAS(SRay Ray, inout SRayPayload Payload, inout int2 Stats)
{
    if (Scene.NumTlasNodes == 0)
    {
        return;
    }

    uint Stack[TLAS_MAX_DEPTH];
    const float MinT = Payload.MinT;

    int StackIndex = 0;
    Stack[StackIndex] = 0;

    while (StackIndex >= 0)
    {
        uint NodeIndex = Stack[StackIndex];
        StackIndex--;

        SBoundingBox Node = TlasNodes[NodeIndex];
        if (asuint(Node.BoxMaxAndNumTriangles.w) > 0)
        {
            uint FirstMeshIndex = asuint(Node.BoxMinAndIndex.w);
            uint LastMeshIndex  = FirstMeshIndex + asuint(Node.BoxMaxAndNumTriangles.w);
            for (uint MeshIndex = FirstMeshIndex; MeshIndex < LastMeshIndex && MeshIndex < Scene.NumMeshes; MeshIndex++)
            {
                HitMesh(Meshes[MeshIndex], Ray, Payload, Stats);
            }
        }
        else
        {
            uint ChildIndex1 = asuint(Node.BoxMinAndIndex.w);
            uint ChildIndex2 = asuint(Node.BoxMinAndIndex.w) + 1;

            float Near1, Far1, Near2, Far2;
            IntersectRayAABBWithFar(TlasNodes[ChildIndex1].BoxMinAndIndex.xyz, TlasNodes[ChildIndex1].BoxMaxAndNumTriangles.xyz, Ray.Origin, Ray.InvDirection, Near1, Far1);
            IntersectRayAABBWithFar(TlasNodes[ChildIndex2].BoxMinAndIndex.xyz, TlasNodes[ChildIndex2].BoxMaxAndNumTriangles.xyz, Ray.Origin, Ray.InvDirection, Near2, Far2);
            Stats.x += 2;

            // Only traverse children that overlap [MinT, Payload.T] (skip nodes entirely behind the ray).
            const bool Hit1 = Near1 < Payload.T && Far1 >= MinT;
            const bool Hit2 = Near2 < Payload.T && Far2 >= MinT;

            if (Near1 > Near2)
            {
                if (Hit1) Stack[++StackIndex] = ChildIndex1;
                if (Hit2) Stack[++StackIndex] = ChildIndex2;
            }
            else
            {
                if (Hit2) Stack[++StackIndex] = ChildIndex2;
                if (Hit1) Stack[++StackIndex] = ChildIndex1;
            }
        }
    }
}

bool TraceRay(SRay Ray, inout SRayPayload Payload, inout int2 Stats)
{
    for (uint i = 0; i < Scene.NumSpheres; i++)
    {
        SSphere Sphere = Spheres[i];
        HitSphere(Sphere, Ray, Payload);
    }

    for (uint i = 0; i < Scene.NumQuads; i++)
    {
        SQuad Quad = Quads[i];
        HitQuad(Quad, Ray, Payload);
    }

    if (Scene.NumTlasNodes > 0)
    {
        TraceTLAS(Ray, Payload, Stats);
    }
    else
    {
        for (uint i = 0; i < Scene.NumMeshes; i++)
        {
            SMesh Mesh = Meshes[i];
            HitMesh(Mesh, Ray, Payload, Stats);
        }
    }

    return Payload.T < Payload.MaxT;
}

#include "trace_backend_software.hlsli"

float3 GetSoftwareBvhIntersectionColor(SRayDesc RayDesc)
{
    SRay Ray;
    Ray.Origin       = RayDesc.Origin;
    Ray.Direction    = RayDesc.Direction;
    Ray.InvDirection = 1.0 / Ray.Direction;

    SRayPayload Payload;
    Payload.MinT        = RayDesc.MinT;
    Payload.MaxT        = RayDesc.MaxT;
    Payload.T           = Payload.MaxT;
    Payload.FrontFace  = 0;
    Payload.FromInside = 0;

    int2 Stats = int2(0, 0);
    TraceRay(Ray, Payload, Stats);

    float3 BoxTestColor      = float3((float)Stats.x, (float)Stats.x, (float)Stats.x) / 100.0;
    float3 TriangleTestColor = float3((float)Stats.y, (float)Stats.y, (float)Stats.y) / 100.0;
    return TriangleTestColor;
}

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void main(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    const bool WritePrimary = (Random.FrameIndex & 1) == 0;
    const int2 Pixel = (int2)DispatchThreadId.xy;

    uint2 Dim;
    OutputTexture.GetDimensions(Dim.x, Dim.y);
    
    const int2 ActualSize = (int2)Dim;

    uint RandomSeed = InitRandom(DispatchThreadId.xy, (uint)ActualSize.x, Random.FrameIndex);

    float2 Jitter = float2(0.0, 0.0);
    if (Scene.ViewMode == VIEW_MODE_RENDER)
    {
        Jitter = float2(NextRandom(RandomSeed), NextRandom(RandomSeed)) - 0.5;
    }

    const float3 CameraPosition = Camera.Position.xyz;
    const float3 FilmTarget     = CalculateFilmTarget(Camera.Position.xyz, Camera.Forward.xyz, Camera.FieldOfViewDegrees, float2(Pixel), float2(ActualSize.xy), Jitter);

    SRay Ray;
    Ray.Origin       = CameraPosition;
    Ray.Direction    = normalize(FilmTarget - CameraPosition);
    Ray.InvDirection = 1.0 / Ray.Direction;

    SRayDesc RayDesc;
    RayDesc.Origin    = Ray.Origin;
    RayDesc.Direction = Ray.Direction;
    RayDesc.MinT      = 0.0001;
    RayDesc.MaxT      = 100000.0;

    const uint MaxBounces = min(Scene.NumBounces, MAX_NUM_BOUNCES) + 1;
    
    bool Accumulate = false;

    float3 SampleColor = EvaluateViewModeColor(Scene.ViewMode, RayDesc, RandomSeed, MaxBounces, Accumulate);
    WriteViewModeOutput(WritePrimary, Pixel, SampleColor, Accumulate, Random.FrameIndex, OutputTexture, PreviousFrameTexture);
}




