#version 450
#include "halton.glsl"
#include "random.glsl"
#include "math.glsl"
#include "primitives.glsl"
#include "utilities.glsl"
#include "ray.glsl"
#include "bvh.glsl"

#define BACKGROUND_TYPE_NONE 0
#define BACKGROUND_TYPE_GRADIENT 1
#define BACKGROUND_TYPE_SKYBOX 2

#define VIEW_MODE_RENDER 0
#define VIEW_MODE_NORMALS 1
#define VIEW_MODE_BARYCENTRICS 2
#define VIEW_MODE_BVH_INTERSECTION 3

#define NUM_THREADS 16
#define MAX_DEPTH 1024
#define SIGMA 0.0001
#define RAY_OFFSET 0.01
#define GAMMA 2.2
#define SKYBOX_MULTIPLIER 1.0

#define ENABLE_QUAD_BACK_FACE_CULLING 1
#define ENABLE_RUSSIAN_ROULETTE 1

layout(local_size_x = NUM_THREADS, local_size_y = NUM_THREADS, local_size_z = 1) in;

layout (binding = 0, rgba32f) uniform image2D uOutput;
layout (binding = 1, rgba32f) uniform image2D uPreviousFrame;

// Bindless Textures
layout (set = 1, binding = 0) uniform samplerCube uTextures[];

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Global uniforms

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
    // 0-16
    uint NumQuads;
    uint NumSpheres;
    uint NumMeshes;
    uint NumMaterials;
    // 16-32
    uint NumBvhNodes;
    uint NumTriangles;
    uint BackgroundType;
    uint NumBounces;
    // 32-36
    uint ViewMode;
    
    // Padding
    uint Padding0;
    uint Padding1;
    uint Padding2;
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
    FVertex Vertices[];
};

layout(std430, binding = 10) buffer VertexExBuffer
{
    FVertexEx VerticesEx[];
};

layout(std430, binding = 11) buffer TriangleBuffer
{
    FTriangle Triangles[];
};

layout(std430, binding = 12) buffer MeshBuffer
{
    FMesh Meshes[];
};

layout(std430, binding = 13) buffer BvhBuffer
{
    FBoundingBox BvhNodes[];
};

float IntersectRayAABB(in uint NodeIndex, in FRay Ray)
{
    vec3 BoxMin = BvhNodes[NodeIndex].BoxMinAndIndex.xyz;
    vec3 BoxMax = BvhNodes[NodeIndex].BoxMaxAndNumTriangles.xyz;
    return IntersectRayAABB(BoxMin, BoxMax, Ray);
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Code

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
        PayLoad.T             = t;
        PayLoad.MaterialIndex = Sphere.MaterialIndex;
        PayLoad.Position      = Ray.Origin + Ray.Direction * PayLoad.T;
        PayLoad.bFromInside   = bFromInside;
        PayLoad.bFrontFace    = !bFromInside; // front face if ray hits from outside
        PayLoad.Normal        = normalize((PayLoad.Position - SpherePos) / SphereRadius) * (bFromInside ? -1.0 : 1.0);
    }
}

bool HitTriangle(in vec3 Vertex0, in vec3 Vertex1, in vec3 Vertex2, in FRay Ray, inout FRayPayLoad PayLoad) 
{
    // Compute the triangle edges
    vec3 Edge1 = Vertex1 - Vertex0;
    vec3 Edge2 = Vertex2 - Vertex0;
    vec3 DirectionCrossEdge2 = cross(Ray.Direction, Edge2);

    float Determinant = dot(Edge1, DirectionCrossEdge2);
    if (abs(Determinant) < SIGMA) 
    {
        return false;
    }

    vec3 RayOriginToVertex0 = Ray.Origin - Vertex0;

    float RecipDeterminant = 1.0 / Determinant;
    float U = RecipDeterminant * dot(RayOriginToVertex0, DirectionCrossEdge2);
    if (U < 0.0 || U > 1.0) 
    {
        return false;
    }

    vec3 RayOriginToVertex0CrossEdge1 = cross(RayOriginToVertex0, Edge1);

    float V = RecipDeterminant * dot(Ray.Direction, RayOriginToVertex0CrossEdge1);
    if (V < 0.0 || U + V > 1.0) 
    {
        return false;
    }

    // At this stage we can compute t to find out where the intersection point is on the line.
    float T = RecipDeterminant * dot(Edge2, RayOriginToVertex0CrossEdge1);
    if (T > PayLoad.MinT && T < PayLoad.MaxT && T < PayLoad.T) 
    {
        PayLoad.T = T;
        PayLoad.BaryCentrics = vec3(U, V, 1.0 - (U + V));
        return true;
    }
    else
    {
        return false;
    }
}

void HitMesh(uint RootBoxIndex, in FRay Ray, inout FRayPayLoad PayLoad, uint MaterialIndex)
{
    // Create a stack for checking all the nodes
    const uint MaxDepth = BVH_MAX_DEPTH;
    uint Stack[MaxDepth];

    // Initialize the stack to visit the rootnode first
    int StackIndex = 0;
    Stack[StackIndex] = RootBoxIndex;

    // Start traversing the bounding boxes
    int LastTriangleHitIndex = -1;
    while (StackIndex >= 0)
    {
        // Pop the stack
        const uint NodeIndex = Stack[StackIndex];
        StackIndex--;

        // Check if we hit this node
        FBoundingBox Node = BvhNodes[NodeIndex];
        if (floatBitsToUint(Node.BoxMaxAndNumTriangles.w) > 0)
        {
            uint LastTriangleIndex = floatBitsToUint(Node.BoxMinAndIndex.w) + floatBitsToUint(Node.BoxMaxAndNumTriangles.w);
            for (uint TriangleIndex = floatBitsToUint(Node.BoxMinAndIndex.w); TriangleIndex < LastTriangleIndex; TriangleIndex++)
            {
                FTriangle Triangle = Triangles[TriangleIndex];
                vec3 Position0 = Vertices[Triangle.Index0].Position.xyz;
                vec3 Position1 = Vertices[Triangle.Index1].Position.xyz;
                vec3 Position2 = Vertices[Triangle.Index2].Position.xyz;

                if (HitTriangle(Position0, Position1, Position2, Ray, PayLoad))
                {
                    vec3 Normal0 = VerticesEx[Triangle.Index0].Normal.xyz;
                    vec3 Normal1 = VerticesEx[Triangle.Index1].Normal.xyz;
                    vec3 Normal2 = VerticesEx[Triangle.Index2].Normal.xyz;

                    PayLoad.Normal = normalize((PayLoad.BaryCentrics.x * Normal1) + (PayLoad.BaryCentrics.y * Normal2) + (PayLoad.BaryCentrics.z * Normal0));
                    LastTriangleHitIndex = int(TriangleIndex);
                }
            }
        }
        else
        {
            uint ChildIndex1 = floatBitsToUint(Node.BoxMinAndIndex.w);
            uint ChildIndex2 = floatBitsToUint(Node.BoxMinAndIndex.w) + 1;

            // Check intersection of child nodes
            float Dist1 = IntersectRayAABB(ChildIndex1, Ray);
            float Dist2 = IntersectRayAABB(ChildIndex2, Ray);

            // Ensure 1 is the closest
            if (Dist1 > Dist2)
            {
                if (Dist1 < PayLoad.T)
                    Stack[++StackIndex] = ChildIndex1;
                if (Dist2 < PayLoad.T)
                    Stack[++StackIndex] = ChildIndex2;
            }
            else
            {
                if (Dist2 < PayLoad.T)
                    Stack[++StackIndex] = ChildIndex2;
                if (Dist1 < PayLoad.T)
                    Stack[++StackIndex] = ChildIndex1;
            }
        }
    }

    if (LastTriangleHitIndex >= 0)
    {
        PayLoad.MaterialIndex = MaterialIndex;
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

bool TraceRay(in FRay Ray, inout FRayPayLoad PayLoad)
{
    for (uint i = 0; i < uScene.NumSpheres; i++)
    {
        FSphere Sphere = Spheres[i];
        HitSphere(Sphere, Ray, PayLoad);
    }

    for (uint i = 0; i < uScene.NumQuads; i++)
    {
        FQuad Quad = Quads[i];
        HitQuad(Quad, Ray, PayLoad);
    }

    for (uint i = 0; i < uScene.NumMeshes; i++)
    {
        FMesh Mesh = Meshes[i];
        HitMesh(Mesh.BoundingBoxIndex, Ray, PayLoad, Mesh.MaterialIndex);
    }

    return PayLoad.T < PayLoad.MaxT;
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
    vec3 CamForward = normalize(uCamera.Forward.xyz);

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
        vec4 SkyboxColor = texture(uTextures[0], UnitDirection);
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
            Ray.Origin       = RayPosition;
            Ray.Direction    = RayDirection;
            Ray.InvDirection = 1.0 / Ray.Direction;
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
        return (PayLoad.Normal + vec3(1.0)) * 0.5;
    }
    else
    {
        return vec3(0.0, 0.0, 0.0);
    }
}

vec3 GetBarycentricsForRay(in FRay Ray)
{
    FRayPayLoad PayLoad;
    PayLoad.MinT        = 0.0001;
    PayLoad.MaxT        = 100000.0;
    PayLoad.T           = PayLoad.MaxT;
    PayLoad.bFrontFace  = false;
    PayLoad.bFromInside = false;

    if (TraceRay(Ray, PayLoad))
    {
        return PayLoad.BaryCentrics;
    }
    else
    {
        return vec3(0.0, 0.0, 0.0);
    }
}

vec3 GetColorForRay_BvhDebug(in FRay Ray)
{
    // Create a stack for checking all the nodes
    const uint MaxDepth = BVH_MAX_DEPTH;
    uint Stack[MaxDepth];

    // Initialize the stack to visit the rootnode first
    int StackIndex = 0;
    Stack[StackIndex] = BVH_ROOT_NODE_INDEX;

    // Payload
    FRayPayLoad PayLoad;
    PayLoad.MinT        = 0.0001;
    PayLoad.MaxT        = 100000.0;
    PayLoad.T           = PayLoad.MaxT;
    PayLoad.bFrontFace  = false;
    PayLoad.bFromInside = false;

    // Start go through all the nodes
    uint NumBoxTests      = 0;
    uint NumTriangleTests = 0;
    while (StackIndex >= 0)
    {
        // Pop the stack
        const uint NodeIndex = Stack[StackIndex];
        StackIndex--;

        // Check if we hit this node
        FBoundingBox Node = BvhNodes[NodeIndex];
        if (floatBitsToUint(Node.BoxMaxAndNumTriangles.w) > 0)
        {
            uint LastTriangleIndex = floatBitsToUint(Node.BoxMinAndIndex.w) + floatBitsToUint(Node.BoxMaxAndNumTriangles.w);
            for (uint TriangleIndex = floatBitsToUint(Node.BoxMinAndIndex.w); TriangleIndex < LastTriangleIndex; TriangleIndex++)
            {
                FTriangle Triangle = Triangles[TriangleIndex];
                vec3 Position0 = Vertices[Triangle.Index0].Position.xyz;
                vec3 Position1 = Vertices[Triangle.Index1].Position.xyz;
                vec3 Position2 = Vertices[Triangle.Index2].Position.xyz;

                HitTriangle(Position0, Position1, Position2, Ray, PayLoad);
                NumTriangleTests++;
            }
        }
        else
        {
            uint ChildIndex1 = floatBitsToUint(Node.BoxMinAndIndex.w);
            uint ChildIndex2 = floatBitsToUint(Node.BoxMinAndIndex.w) + 1;

            // Check intersection of child nodes
            float Dist1 = IntersectRayAABB(ChildIndex1, Ray);
            float Dist2 = IntersectRayAABB(ChildIndex2, Ray);
            NumBoxTests += 2;

            // Ensure 1 is the closest
            if (Dist1 > Dist2)
            {
                if (Dist1 < PayLoad.T)
                    Stack[++StackIndex] = ChildIndex1;
                if (Dist2 < PayLoad.T)
                    Stack[++StackIndex] = ChildIndex2;
            }
            else
            {
                if (Dist2 < PayLoad.T)
                    Stack[++StackIndex] = ChildIndex2;
                if (Dist1 < PayLoad.T)
                    Stack[++StackIndex] = ChildIndex1;
            }
        }
    }

    vec3 BoxTestColor      = vec3(float(NumBoxTests)) / 50.0;
    vec3 TriangleTestColor = vec3(float(NumTriangleTests)) / 50.0;
    return BoxTestColor;
}

void main()
{
    const ivec2 Pixel = ivec2(gl_GlobalInvocationID.xy);
    const ivec2 Size  = ivec2(gl_NumWorkGroups.xy * gl_WorkGroupSize.xy);

    // Jitter the camera each frame
    uint RandomSeed = InitRandom(uvec2(Pixel), uint(Size.x), uRandom.FrameIndex);

    vec2 Jitter = vec2(NextRandom(RandomSeed), NextRandom(RandomSeed)) - 0.5;
    const vec3 CameraPosition = uCamera.Position.xyz;
    const vec3 FilmTarget = CalculateFilmTarget(Pixel, Size, Jitter);

    // Setup the first Ray
    FRay Ray;
    Ray.Origin       = CameraPosition;
    Ray.Direction    = normalize(FilmTarget - CameraPosition);
    Ray.InvDirection = 1.0 / Ray.Direction;

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
        else if (uScene.ViewMode == VIEW_MODE_BARYCENTRICS)
    {
        // Get Barycentrics for this Ray
        vec3 HitBarycentrics = GetBarycentricsForRay(Ray);
        imageStore(uOutput, Pixel, vec4(HitBarycentrics, 1.0));
    }
    else if (uScene.ViewMode == VIEW_MODE_BVH_INTERSECTION)
    {
        // Get Color for this Ray
        vec3 Color = GetColorForRay_BvhDebug(Ray);
        imageStore(uOutput, Pixel, vec4(Color, 1.0));
    }
}
