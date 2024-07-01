#version 450
#include "halton.glsl"
#include "random.glsl"
#include "math.glsl"
#include "primitives.glsl"
#include "ray.glsl"
#include "bvh.glsl"

#define BACKGROUND_TYPE_NONE 0
#define BACKGROUND_TYPE_GRADIENT 1
#define BACKGROUND_TYPE_SKYBOX 2

#define VIEW_MODE_RENDER 0
#define VIEW_MODE_NORMALS 1
#define VIEW_MODE_TOP_BVH 2

#define NUM_THREADS 16
#define MAX_DEPTH 1024
#define SIGMA 0.0001
#define RAY_OFFSET 0.01
#define GAMMA 2.2
#define SKYBOX_MULTIPLIER 1.0

#define ENABLE_QUAD_BACK_FACE_CULLING 1
#define ENABLE_TRIANGLE_BACK_FACE_CULLING 0
#define ENABLE_RUSSIAN_ROULETTE 1

layout(local_size_x = NUM_THREADS, local_size_y = NUM_THREADS, local_size_z = 1) in;

layout (binding = 0, rgba32f) uniform image2D uOutput;
layout (binding = 1, rgba32f) uniform image2D uPreviousFrame;
layout (binding = 2)          uniform samplerCube uSkybox;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Global uniforms

layout(binding = 3) uniform CameraBufferObject 
{
    // 0-64
    mat4 Projection;
    // 64-128
    mat4 View;
    // 128-160
    vec4 Position;
    vec4 Forward;
    // 160-164
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
    // 0-16
    uint NumQuads;
    uint NumSpheres;
    uint NumTriangleMeshes;
    uint NumMaterials;
    // 16-32
    uint NumBvhNodes;
    uint BackgroundType;
    uint NumBounces;
    uint ViewMode;
} uScene;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Scene objects

layout(std430, binding = 6) buffer QuadBuffer
{
    FQuad Quads[];
};

layout(std430, binding = 7) buffer SphereBuffer
{
    FSphere Spheres[];
};

layout(std430, binding = 8) buffer MaterialBuffer
{
    FMaterial Materials[];
};

layout(std430, binding = 9) buffer VertexBuffer
{
    FVertexRT Vertices[];
};

layout(std430, binding = 10) buffer TriangleBuffer
{
    FTriangle Triangles[];
};

layout(std430, binding = 11) buffer TriangleMeshBuffer
{
    FTriangleMesh TriangleMeshes[];
};

layout(std430, binding = 12) buffer BvhBuffer
{
    FBvhNode BvhNodes[];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Code

vec3 HemisphereSampleUniform(float u, float v) 
{
    float phi      = v * 2.0 * PI;
    float cosTheta = 1.0 - u;
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    return normalize(vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta));
}

bool IsAlmostZero(vec3 Value)
{
    return Value.x <= SIGMA && Value.y <= SIGMA && Value.z <= SIGMA; 
}

void HitQuad(in FQuad Quad, in FRay Ray, inout FRayPayLoad PayLoad)
{
    vec3 Q = Quad.Position.xyz;
    vec3 U = Quad.Edge0.xyz;
    vec3 V = Quad.Edge1.xyz;
    vec3 N = cross(U, V);
    vec3 W = N / dot(N, N);

    vec3  Normal = normalize(N);
    float D = dot(Normal, Q);

    float DdotN = dot(Ray.Direction, Normal);

    // Back-face culling: skip if the dot product is positive (back face)
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
            vec3 Intersection = Ray.Origin + (Ray.Direction * t);
            vec3 PlanarHit    = Intersection - Q;
            float Alpha = dot(W, cross(PlanarHit, V));
            float Beta  = dot(W, cross(U, PlanarHit));
            if (Alpha < 0.0 || 1.0 < Alpha || Beta < 0.0 || 1.0 < Beta)
            {
                return;
            }

            PayLoad.T             = t;
            PayLoad.MaterialIndex = Quad.MaterialIndex;
            PayLoad.bFrontFace    = true;
            PayLoad.bFromInside   = false;
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

void HitSphere(in FSphere Sphere, in FRay Ray, inout FRayPayLoad PayLoad)
{
    // Extract sphere position and radius
    vec3 SpherePos = Sphere.PositionAndRadius.xyz;
    float SphereRadius = Sphere.PositionAndRadius.w;

    // Vector from ray origin to sphere center
    vec3 oc = Ray.Origin - SpherePos;

    // Coefficients for the quadratic equation (a*t^2 + 2*b*t + c = 0)
    float a = dot(Ray.Direction, Ray.Direction); // Direction should be normalized, so a is usually 1
    float b = dot(Ray.Direction, oc); // Note that this is b' which is b/2 in some formulations
    float c = dot(oc, oc) - (SphereRadius * SphereRadius);

    // Discriminant of the quadratic equation
    float Discriminant = (b * b) - a * c;

    // If the discriminant is negative, there are no real roots, hence no intersection
    if (Discriminant < 0.0)
    {
        return;
    }

    // Calculate the first intersection point (nearest point)
    float sqrtDiscriminant = sqrt(Discriminant);
    float t = (-b - sqrtDiscriminant) / a;

    bool bFromInside = false;

    // Check if the first intersection point is within the valid range
    if (t <= PayLoad.MinT || t >= PayLoad.MaxT)
    {
        // Calculate the second intersection point (farther point)
        t = (-b + sqrtDiscriminant) / a;
        bFromInside = true;

        // Check if the second intersection point is within the valid range
        if (t <= PayLoad.MinT || t >= PayLoad.MaxT)
        {
            return;
        }
    }

    // If this intersection point is closer than the previous hit, update the payload
    if (t <= PayLoad.T)
    {
        PayLoad.T = t;
        PayLoad.MaterialIndex = Sphere.MaterialIndex;
        PayLoad.Position = Ray.Origin + Ray.Direction * PayLoad.T;
        PayLoad.bFromInside = bFromInside;
        PayLoad.bFrontFace = !bFromInside; // front face if ray hits from outside
        PayLoad.Normal = normalize((PayLoad.Position - SpherePos) / SphereRadius) * (bFromInside ? -1.0 : 1.0);
    }
}

void HitTriangle(in vec3 Vertex0, in vec3 Vertex1, in vec3 Vertex2, in FRay Ray, inout FRayPayLoad PayLoad, uint MaterialIndex) 
{
    // Compute the triangle edges
    vec3 Edge1 = Vertex1 - Vertex0;
    vec3 Edge2 = Vertex2 - Vertex0;

    // Compute the determinant between the 
    vec3  DirectionCrossEdge2 = cross(Ray.Direction, Edge2);
    float Determinant = dot(Edge1, DirectionCrossEdge2);

    // If the determinant is almost zero that means that the Ray is parallell to the triangle and we early return
    if (abs(Determinant) < SIGMA) 
    {
        return;
    }

    // Back-face culling: skip if the dot product is positive (back face)
    vec3 Normal = normalize(cross(Edge1, Edge2));
    float DdotN = dot(Ray.Direction, Normal);
#if ENABLE_TRIANGLE_BACK_FACE_CULLING
    if (DdotN > 0.0)
    {
        return;
    }
#endif

    // Calculate the inverse determinant
    float InvDeterminant = 1.0 / Determinant;

    // Calculate vector from Ray origin to vertex0
    vec3 RayOriginToVertex0 = Ray.Origin - Vertex0;

    // If u is outside the range [0, 1], the intersection point is outside the triangle
    float u = InvDeterminant * dot(RayOriginToVertex0, DirectionCrossEdge2);
    if (u < 0.0 || u > 1.0) 
    {
        return;
    }

    vec3 RayOriginToVertex0CrossEdge1 = cross(RayOriginToVertex0, Edge1);

    float v = InvDeterminant * dot(Ray.Direction, RayOriginToVertex0CrossEdge1);
    if (v < 0.0 || u + v > 1.0) 
    {
        return;
    }

    // At this stage we can compute t to find out where the intersection point is on the line.
    float t = InvDeterminant * dot(Edge2, RayOriginToVertex0CrossEdge1);
    if (t > PayLoad.MinT && t < PayLoad.MaxT && t < PayLoad.T) 
    {
        PayLoad.T             = t;
        PayLoad.MaterialIndex = MaterialIndex;
        PayLoad.Position      = Ray.Origin + t * Ray.Direction;
        PayLoad.bFromInside   = false;

        if (DdotN < 0.0) 
        {
            PayLoad.Normal     = Normal;
            PayLoad.bFrontFace = true;
        }
        else
        {
            PayLoad.Normal     = -Normal;
            PayLoad.bFrontFace = false;
        }
    }
}

bool IntersectRayAABB(in vec3 BoxMin, in vec3 BoxMax, in FRay Ray)
{
    vec3 MinT = (BoxMin - Ray.Origin) / Ray.Direction;
    vec3 MaxT = (BoxMax - Ray.Origin) / Ray.Direction;

    vec3 T1 = min(MinT, MaxT);
    vec3 T2 = max(MinT, MaxT);

    float NearT = max(max(T1.x, T1.y), T1.z);
    float FarT  = min(min(T2.x, T2.y), T2.z);
    return FarT >= max(NearT, 0.0);
}

bool TraceRay(in FRay Ray, inout FRayPayLoad PayLoad)
{
    for (uint i = 0; i < uScene.NumBvhNodes; i++)
    {
        FBvhNode Node = BvhNodes[i];
        if (Node.ObjectType == OBJECT_TYPE_SPHERE)
        {
            const uint SphereIndex = Node.ObjectIndex;
            FSphere Sphere = Spheres[SphereIndex];
            HitSphere(Sphere, Ray, PayLoad);
        }
        else if (Node.ObjectType == OBJECT_TYPE_QUAD)
        {
            const uint QuadIndex = Node.ObjectIndex;
            FQuad Quad = Quads[QuadIndex];
            HitQuad(Quad, Ray, PayLoad);
        }
    }

    for (uint i = 0; i < uScene.NumTriangleMeshes; i++)
    {
        FTriangleMesh Mesh = TriangleMeshes[i];

        // Only test each triangle if we actually intersect the bounding box
        if (!IntersectRayAABB(Mesh.BoxMin.xyz, Mesh.BoxMax.xyz, Ray))
        {
            continue;
        }

        // Test each triangle in the mesh 
        uint StartTriangle = Mesh.StartTriangle;
        uint EndTriangle = StartTriangle + Mesh.NumTriangles;
        for (uint j = StartTriangle; j < EndTriangle; j++)
        {
            FTriangle Triangle = Triangles[j];
            vec3 Pos0 = Vertices[Triangle.Index0].Position.xyz;
            vec3 Pos1 = Vertices[Triangle.Index1].Position.xyz;
            vec3 Pos2 = Vertices[Triangle.Index2].Position.xyz;

            HitTriangle(Pos0, Pos1, Pos2, Ray, PayLoad, Mesh.MaterialIndex);
        }
    }

    if (PayLoad.T < PayLoad.MaxT)
    {
        return true;
    }
    else
    {
        return false;
    }
}

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

vec3 CalculateFilmTarget(ivec2 Pixel, ivec2 Size, vec2 Jitter)
{
    vec3 CameraPosition = uCamera.Position.xyz;
    vec3 CamForward     = normalize(uCamera.Forward.xyz);
    
    vec3 CamUp = vec3(0.0, 1.0, 0.0);
    CamUp = normalize(CamUp - dot(CamUp, CamForward) * CamForward);
    vec3 CamRight = normalize(cross(CamUp, CamForward));

    float AspectRatio  = float(Size.x) / float(Size.y);
    float FieldOfView  = clamp(uCamera.FieldOfViewDegrees, 30.0, 120.0);
    float FilmDistance = 1.0 / tan(FieldOfView * 0.5 * PI / 180.0); 
    vec3  FilmCenter   = CameraPosition + (CamForward * FilmDistance);

    vec2 FilmUV = (vec2(Pixel) + Jitter) / vec2(Size.xy);
    FilmUV.y = 1.0 - FilmUV.y;
    FilmUV   = FilmUV * 2.0;

    vec2 FilmCorner = vec2(-1.0, -1.0);
    vec2 FilmCoord  = FilmCorner + FilmUV;
    FilmCoord.x = FilmCoord.x * AspectRatio;
    return FilmCenter + (CamRight * FilmCoord.x) + (CamUp * FilmCoord.y);
}

vec3 GetEnvironmentLight(vec3 RayDirection)
{
    if (uScene.BackgroundType == BACKGROUND_TYPE_NONE)
    {
        // Only light source is the emissive surfaces
        return vec3(0.0);
    }
    else if (uScene.BackgroundType == BACKGROUND_TYPE_GRADIENT)
    {
        // Create a gradient
        vec3 UnitDirection = normalize(RayDirection);
        float Alpha = 0.5 * (UnitDirection.y + 1.0);
        return (1.0 - Alpha) * vec3(1.0, 1.0, 1.0) + Alpha * vec3(0.5, 0.7, 1.0);
    }
    else if (uScene.BackgroundType == BACKGROUND_TYPE_SKYBOX)
    {
        // Sample the Skybox
        vec3 UnitDirection = normalize(RayDirection);
        vec4 SkyboxColor = texture(uSkybox, UnitDirection);
        return SkyboxColor.rgb * SKYBOX_MULTIPLIER;
    }
    else
    {
        return vec3(0.0, 0.0, 0.0);
    }
}

vec3 GetColorForRay(in FRay Ray, inout uint RandomSeed)
{
    // Start tracing rays
    vec3 RayColor    = vec3(1.0);
    vec3 SampleColor = vec3(0.0);

    uint MaxBounces = min(uScene.NumBounces, MAX_DEPTH);
    for (uint i = 0; i < MaxBounces; i++)
    {
        FRayPayLoad PayLoad;
        PayLoad.MinT        = 0.0001;
        PayLoad.MaxT        = 100000.0;
        PayLoad.T           = PayLoad.MaxT;
        PayLoad.bFrontFace  = false;
        PayLoad.bFromInside = false;

        if (TraceRay(Ray, PayLoad))
        {
            const uint MaterialIndex = min(PayLoad.MaterialIndex, uScene.NumMaterials - 1);
            FMaterial Material = Materials[MaterialIndex];

            if (PayLoad.bFromInside)
            {
                RayColor *= exp(-Material.AbsorbtionColor.rgb * PayLoad.T);
            }

            float SpecularChance   = Material.SpecularChance;
            float RefractionChance = Material.RefractionChance;
            
            if (SpecularChance > 0.0)
            {
                float IncidenceOfRefraction1 = PayLoad.bFromInside ? Material.IncidenceOfRefraction : 1.0;
                float IncidenceOfRefraction2 = !PayLoad.bFromInside ? Material.IncidenceOfRefraction : 1.0;
                SpecularChance = FresnelReflectAmount(IncidenceOfRefraction1, IncidenceOfRefraction2, Ray.Direction, PayLoad.Normal, Material.SpecularChance, 1.0);

                float ChanceMultiplier = (1.0 - SpecularChance) / (1.0 - Material.SpecularChance);
                RefractionChance *= ChanceMultiplier;
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

            vec3 RayDirection = Ray.Direction;
            vec3 RayPosition  = PayLoad.Position;
            if (DoRefraction == 1.0)
            {
                RayPosition = RayPosition - PayLoad.Normal * RAY_OFFSET;
            }
            else
            {
                RayPosition = RayPosition + PayLoad.Normal * RAY_OFFSET;
            }

            // Create diffuse ray
            vec3 DiffuseRay = normalize(PayLoad.Normal + NextRandomUnitSphereVec3(RandomSeed));

            // Create specular ray 
            vec3 SpecularRay = reflect(RayDirection, PayLoad.Normal);
            SpecularRay = normalize(mix(SpecularRay, DiffuseRay, Material.SpecularRoughness * Material.SpecularRoughness));
            
            // Create refraction ray
            vec3 RefractionRay = refract(RayDirection, PayLoad.Normal, PayLoad.bFromInside ? Material.IncidenceOfRefraction : 1.0 / Material.IncidenceOfRefraction);
            RefractionRay = normalize(mix(RefractionRay, normalize(PayLoad.Normal + NextRandomUnitSphereVec3(RandomSeed)), Material.RefractionRoughness * Material.RefractionRoughness));

            // blend rays
            RayDirection = mix(DiffuseRay, SpecularRay, DoSpecular);
            RayDirection = mix(RayDirection, RefractionRay, DoRefraction);

            SampleColor += Material.EmissiveColor.rgb * RayColor;

            if (DoRefraction == 0.0)
            {
                RayColor *= mix(Material.AlbedoColor.rgb, Material.SpecularColor.rgb, DoSpecular);
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

            // Setup the next Ray
            Ray.Origin    = RayPosition;
            Ray.Direction = RayDirection;
        }
        else
        {
            // Add this hit color
            SampleColor += GetEnvironmentLight(Ray.Direction) * RayColor;
            break;
        }
    }

    return SampleColor;
}

vec3 GetNormalForRay(in FRay Ray)
{
    FRayPayLoad PayLoad;
    PayLoad.MinT        = 0.0001;
    PayLoad.MaxT        = 100000.0;
    PayLoad.T           = PayLoad.MaxT;
    PayLoad.bFrontFace  = false;
    PayLoad.bFromInside = false;

    if (TraceRay(Ray, PayLoad))
    {
        return PayLoad.Normal;
    }
    else
    {
        return vec3(0.0, 0.0, 0.0);
    }
}

vec3 GetColorForRay_BvhDebug(in FRay Ray)
{
    uint NumHits = 0;
    for (uint i = 0; i < uScene.NumBvhNodes; i++)
    {
        FBvhNode Node = BvhNodes[i];
        if (IntersectRayAABB(Node.AABBMin.xyz, Node.AABBMax.xyz, Ray))
        {
            NumHits++;
        }
    }

    if (NumHits > 0)
    {
        float Color = min(0.1 + (float(NumHits) / float(uScene.NumBvhNodes)), 1.0);
        return vec3(Color, Color, Color);
    }
    else
    {
        return vec3(0.0, 0.0, 0.0);
    }
}

void main()
{
    const ivec2 Pixel = ivec2(gl_GlobalInvocationID.xy);
    const ivec2 Size  = ivec2(gl_NumWorkGroups.xy * gl_WorkGroupSize.xy);

    // Jitter the camera each frame
    uint RandomSeed = InitRandom(uvec2(Pixel), uint(Size.x), uRandom.FrameIndex);

    vec2 Jitter = vec2(NextRandom(RandomSeed), NextRandom(RandomSeed)) - 0.5;
    const vec3 CameraPosition = uCamera.Position.xyz;
    const vec3 FilmTarget     = CalculateFilmTarget(Pixel, Size, Jitter);

    // Setup the first Ray
    FRay Ray;
    Ray.Origin    = CameraPosition;
    Ray.Direction = normalize(FilmTarget - CameraPosition);

    if (uScene.ViewMode == VIEW_MODE_RENDER)
    {
        // Get Color for this Ray
        vec3 SampleColor = GetColorForRay(Ray, RandomSeed);

        // Accumulate samples over time
        vec4 PreviousColor = imageLoad(uPreviousFrame, Pixel);
        vec3 CurrentColor  = mix(PreviousColor.rgb, SampleColor, 1.0 / float(uRandom.FrameIndex + 1));
        imageStore(uOutput, Pixel, vec4(CurrentColor, 1.0));
    }
    else if (uScene.ViewMode == VIEW_MODE_NORMALS)
    {
        // Get Normal for this Ray
        vec3 HitNormal = GetNormalForRay(Ray);
        imageStore(uOutput, Pixel, vec4(HitNormal, 1.0));
    }
    else if (uScene.ViewMode == VIEW_MODE_TOP_BVH)
    {
        // Get Color for this Ray
        vec3 Color = GetColorForRay_BvhDebug(Ray);
        imageStore(uOutput, Pixel, vec4(Color, 1.0));
    }
}
