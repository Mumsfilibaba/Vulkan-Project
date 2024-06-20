#version 450
#include "halton.glsl"
#include "random.glsl"
#include "math.glsl"

#define BACKGROUND_TYPE_NONE 0
#define BACKGROUND_TYPE_GRADIENT 1
#define BACKGROUND_TYPE_SKYBOX 2

#define NUM_THREADS 16
#define MAX_DEPTH 1024
#define SIGMA 0.0001
#define GAMMA 2.2

#define ENABLE_RAY_OFFSET 0
#define ENABLE_QUAD_BACK_FACE_CULLING 1
#define ENABLE_TRIANGLE_BACK_FACE_CULLING 0

layout(local_size_x = NUM_THREADS, local_size_y = NUM_THREADS, local_size_z = 1) in;

layout (binding = 0, rgba32f) uniform image2D uOutput;
layout (binding = 1, rgba32f) uniform image2D uPreviousFrame;
layout (binding = 2)          uniform samplerCube uSkybox;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* Global uniforms */

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
    uint NumPlanes;
    uint NumMaterials;
    // 16-28
    uint NumTriangleMeshes;
    uint BackgroundType;
    uint NumBounces;
    // Padding
    uint Padding0;
} uScene;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* Scene objects */

struct FMaterial
{
    vec4  AlbedoColor;
    vec4  EmissiveColor;
    vec4  SpecularColor;
    float SpecularFactor;
    float Roughness;
    float RefractionIndex;
    uint  Padding0;
};

struct FQuad
{
    vec4 Position;
    vec4 Edge0;
    vec4 Edge1;
    uint MaterialIndex;
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

struct FSphere
{
    vec4 PositionAndRadius;
    uint MaterialIndex;
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

struct FPlane 
{
    vec4 NormalAndDistance;
    uint MaterialIndex;
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

struct FVertexRT
{
    vec4 Position;
};

struct FTriangle
{
    uint Index0;
    uint Index1;
    uint Index2;
    uint Padding0;
};

struct FTriangleMesh
{
    vec4 BoxMin;
    vec4 BoxMax;
    uint StartTriangle;
    uint NumTriangles;
    uint Padding0;
    uint Padding1;
};

layout(std430, binding = 6) buffer QuadBuffer
{
    FQuad Quads[];
};

layout(std430, binding = 7) buffer SphereBuffer
{
    FSphere Spheres[];
};

layout(std430, binding = 8) buffer PlaneBuffer
{
    FPlane Planes[];
};

layout(std430, binding = 9) buffer MaterialBuffer
{
    FMaterial Materials[];
};

layout(std430, binding = 10) buffer VertexBuffer
{
    FVertexRT Vertices[];
};

layout(std430, binding = 11) buffer TriangleBuffer
{
    FTriangle Triangles[];
};

layout(std430, binding = 12) buffer TriangleMeshBuffer
{
    FTriangleMesh TriangleMeshes[];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* Ray Structs */

struct FRay
{
    vec3 Origin;
    vec3 Direction;
};

struct FRayPayLoad
{
    vec3  Normal;
    vec3  Position;
    float T;
    float MinT;
    float MaxT;
    bool  FrontFace;
    uint  MaterialIndex;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* Code */

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
            PayLoad.FrontFace     = true;
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
    vec3  SpherePos    = Sphere.PositionAndRadius.xyz;
    float SphereRadius = Sphere.PositionAndRadius.w;

    vec3  oc = Ray.Origin - SpherePos;
    float a = dot(Ray.Direction, Ray.Direction);
    float b = dot(Ray.Direction, oc);
    float c = dot(oc, oc) - (SphereRadius * SphereRadius);

    float Discriminant = (b * b) - (a * c);
    if (Discriminant < 0.0)
    {
        return;
    }

    float t = (-b - sqrt(Discriminant)) / a;
    if (t <= PayLoad.MinT || t >= PayLoad.MaxT)
    {
        t = (-b + sqrt(Discriminant)) / a;
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

        vec3 OutsideNormal = normalize((PayLoad.Position - SpherePos) / SphereRadius);
        if (dot(Ray.Direction, OutsideNormal) < 0.0)
        {
            PayLoad.Normal    = OutsideNormal;
            PayLoad.FrontFace = true;
        }
        else
        {
            PayLoad.Normal    = -OutsideNormal;
            PayLoad.FrontFace = false;
        }
    }
}

void HitPlane(in FPlane Plane, in FRay Ray, inout FRayPayLoad PayLoad)
{
    vec3  PlaneNormal = normalize(Plane.NormalAndDistance.xyz);
    float PlaneDist   = Plane.NormalAndDistance.w;

    float DdotN = dot(Ray.Direction, PlaneNormal);
    if (abs(DdotN) < SIGMA)
    {
        return;
    }

    vec3 Center = PlaneNormal * PlaneDist;
    vec3 Diff   = Center - Ray.Origin;

    float t = dot(Diff, PlaneNormal) / DdotN;
    if (t > 0.0)
    {
        if (t < PayLoad.T)
        {
            PayLoad.T             = t;
            PayLoad.MaterialIndex = Plane.MaterialIndex;
            PayLoad.FrontFace     = true;
            PayLoad.Position      = Ray.Origin + Ray.Direction * PayLoad.T;

            if (DdotN >= 0.0)
            {
                PayLoad.Normal = -PlaneNormal;
            }
            else
            {
                PayLoad.Normal = PlaneNormal;
            }
        }
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
        
        if (DdotN < 0.0) 
        {
            PayLoad.Normal    = Normal;
            PayLoad.FrontFace = true;
        }
        else
        {
            PayLoad.Normal    = -Normal;
            PayLoad.FrontFace = false;
        }
    }
}

bool IntersectRayAABB(in vec3 BoxMin, in vec3 BoxMax, in FRay Ray) 
{
    // Initialize MinT and MaxT to the full range
    float MinT = (BoxMin.x - Ray.Origin.x) / Ray.Direction.x;
    float MaxT = (BoxMax.x - Ray.Origin.x) / Ray.Direction.x;

    // Swap MinT and MaxT if needed
    if (MinT > MaxT)
    {
        float Temp = MinT;
        MinT = MaxT;
        MaxT = Temp;
    }

    float MinTy = (BoxMin.y - Ray.Origin.y) / Ray.Direction.y;
    float MaxTy = (BoxMax.y - Ray.Origin.y) / Ray.Direction.y;

    // Swap MinTy and MaxTy if needed
    if (MinTy > MaxTy)
    {
        float Temp = MinTy;
        MinTy = MaxTy;
        MaxTy = Temp;
    }

    // Check for overlap in the y-direction
    if (MinT > MaxTy || MinTy > MaxT)
    {
        return false;
    }

    // Update MinT and MaxT to account for y-axis overlap
    if (MinTy > MinT)
    {
        MinT = MinTy;
    }

    if (MaxTy < MaxT)
    {
        MaxT = MaxTy;
    }

    float MinTz = (BoxMin.z - Ray.Origin.z) / Ray.Direction.z;
    float MaxTz = (BoxMax.z - Ray.Origin.z) / Ray.Direction.z;

    // Swap MinTz and MaxTz if needed
    if (MinTz > MaxTz)
    {
        float Temp = MinTz;
        MinTz = MaxTz;
        MaxTz = Temp;
    }

    // Check for overlap in the z-direction
    if (MinT > MaxTz || MinTz > MaxT)
    {
        return false;
    }

    // Update MinT and MaxT to account for z-axis overlap
    if (MinTz > MinT)
    {
        MinT = MinTz;
    }
    if (MaxTz < MaxT)
    {
        MaxT = MaxTz;
    }

    // If we reach this point, there is an intersection
    return true;
}

bool TraceRay(in FRay Ray, inout FRayPayLoad PayLoad)
{
    for (uint i = 0; i < uScene.NumQuads; i++)
    {
        FQuad Quad = Quads[i];
        HitQuad(Quad, Ray, PayLoad);
    }

    for (uint i = 0; i < uScene.NumSpheres; i++)
    {
        FSphere Sphere = Spheres[i];
        HitSphere(Sphere, Ray, PayLoad);
    }

    for (uint i = 0; i < uScene.NumPlanes; i++)
    {
        FPlane Plane = Planes[i];
        HitPlane(Plane, Ray, PayLoad);
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

            HitTriangle(Pos0, Pos1, Pos2, Ray, PayLoad, 0);
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
        return SkyboxColor.rgb;
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

    vec2 Jitter = Halton23(uRandom.HaltonIndex);
    Jitter = (Jitter * 2.0) - vec2(1.0);

    const vec3 CameraPosition = uCamera.Position.xyz;
    const vec3 FilmTarget     = CalculateFilmTarget(Pixel, Size, Jitter);

    // Setup the first Ray
    FRay Ray;
    Ray.Origin    = CameraPosition;
    Ray.Direction = normalize(FilmTarget - CameraPosition);

    // Start tracing rays
    vec3 RayColor    = vec3(1.0);
    vec3 SampleColor = vec3(0.0);

    uint MaxBounces = min(uScene.NumBounces, MAX_DEPTH);
    for (uint i = 0; i < MaxBounces; i++)
    {
        FRayPayLoad PayLoad;
        PayLoad.MinT = 0.001;
        PayLoad.MaxT = 1000.0;
        PayLoad.T    = PayLoad.MaxT;

        if (TraceRay(Ray, PayLoad))
        {
            const uint MaterialIndex = min(PayLoad.MaterialIndex, uScene.NumMaterials - 1);
            FMaterial Material = Materials[MaterialIndex];

            vec3 Rnd       = NextRandomUnitSphereVec3(RandomSeed);
            vec3 Direction = normalize(PayLoad.Normal + Rnd);
            vec3 Origin    = PayLoad.Position;

            vec3 EmissiveColor = Material.EmissiveColor.rgb * RayColor;
            SampleColor += EmissiveColor;

            RayColor *= Material.AlbedoColor.rgb * RayColor;

            // Setup the next Ray
            Ray.Origin    = Origin;
            Ray.Direction = Direction;
        }
        else
        {
            // Add this hit color
            vec3 EnvironmentLight = GetEnvironmentLight(Ray.Direction) * RayColor;
            SampleColor += EnvironmentLight;
            break;
        }
    }

    // Accumulate samples over time
    vec4 PreviousColor = imageLoad(uPreviousFrame, Pixel);
    vec3 CurrentColor  = mix(PreviousColor.rgb, SampleColor, 1.0 / float(uRandom.FrameIndex + 1));
    imageStore(uOutput, Pixel, vec4(CurrentColor, 1.0));
}

#if 0
void main()
{
    const ivec2 Pixel = ivec2(gl_GlobalInvocationID.xy);
    const ivec2 Size  = ivec2(gl_NumWorkGroups.xy * gl_WorkGroupSize.xy);

    // Jitter the camera each frame
    uint RandomSeed = InitRandom(uvec2(Pixel), uint(Size.x), uRandom.FrameIndex);

    vec2 Jitter = Halton23(uRandom.SampleIndex);
    Jitter = (Jitter * 2.0) - vec2(1.0);

    const vec3 CameraPosition = uCamera.Position.xyz;
    const vec3 FilmTarget     = CalculateFilmTarget(Pixel, Size, Jitter);

    // Setup the first Ray
    FRay Ray;
    Ray.Origin    = CameraPosition;
    Ray.Direction = normalize(FilmTarget - CameraPosition);

    // Start tracing rays
    vec3 SampleColor = vec3(1.0);

    uint MaxBounces = min(uScene.NumBounces, MAX_DEPTH);
    for (uint i = 0; i < MaxBounces; i++)
    {
        FRayPayLoad PayLoad;
        PayLoad.MinT = 0.001;
        PayLoad.MaxT = 1000.0;
        PayLoad.T    = PayLoad.MaxT;

        if (TraceRay(Ray, PayLoad))
        {
            const uint MaterialIndex = min(PayLoad.MaterialIndex, uScene.NumMaterials - 1);
            FMaterial Material = Materials[MaterialIndex];
            
            vec3 N        = normalize(PayLoad.Normal);
            vec3 EmissiveColor  = vec3(0.0);
            vec3 Origin    = vec3(0.0);
            vec3 Direction = vec3(0.0);

            if (Material.Type == MATERIAL_LAMBERTIAN)
            {
                vec3 Rnd = NextRandomUnitSphereVec3(RandomSeed);
                Direction = normalize(PayLoad.Normal + Rnd);

            #if ENABLE_RAY_OFFSET
                Origin = PayLoad.Position + (N * SIGMA);
            #else
                Origin = PayLoad.Position;
            #endif

                /*if (IsAlmostZero(Direction))
                {
                    Direction = PayLoad.Normal;
                }*/

                // Attenuate light
                vec3 AlbedoColor = min(Material.AlbedoColor.rgb, vec3(0.99));
                SampleColor = AlbedoColor * SampleColor;
            }
            else if (Material.Type == MATERIAL_METAL)
            {
                vec3 Rnd = NextRandomHemisphere(RandomSeed, PayLoad.Normal);

                vec3 Reflection = reflect(Ray.Direction, N);
                Direction = normalize(Reflection + Rnd * Material.Roughness);
            #if ENABLE_RAY_OFFSET
                Origin = PayLoad.Position + (N * SIGMA);
            #else
                Origin = PayLoad.Position;
            #endif

                // Attenuate light
                vec3 AlbedoColor = min(Material.AlbedoColor.rgb, vec3(0.99));
                SampleColor = AlbedoColor * SampleColor;
            }
            else if (Material.Type == MATERIAL_DIELECTRIC)
            {
                float RefractionRatio = PayLoad.FrontFace ? (1.0 / max(Material.RefractionIndex, SIGMA)) : Material.RefractionIndex;

                vec3  RayDirection = normalize(Ray.Direction); 
                float CosTheta = min(dot(-RayDirection, PayLoad.Normal), 1.0);
                float SinTheta = sqrt(1.0 - CosTheta * CosTheta);

                bool bShouldReflect = RefractionRatio * SinTheta >= 1.0;
                if (bShouldReflect || Reflectance(CosTheta, RefractionRatio) > NextRandom(RandomSeed))
                {
                    vec3 Rnd = NextRandomHemisphere(RandomSeed, PayLoad.Normal);

                    vec3 Reflection = reflect(RayDirection, N);
                    Direction = normalize(Reflection + Rnd * Material.Roughness);
                }
                else
                {
                    // TODO: The GLSL refract seems to give NaN sometimes
                #if 0
                    vec3 Refracted = refract(RayDirection, N, RefractionRatio);
                #else
                    vec3 Refracted = RealRefract(RayDirection, N, RefractionRatio);
                #endif
                    Direction = Refracted;
                }
                
            #if ENABLE_RAY_OFFSET
                if (PayLoad.FrontFace)
                {
                    Origin = PayLoad.Position + (N * SIGMA);
                }
                else
                {
                    Origin = PayLoad.Position - (N * SIGMA);
                }
            #else
                Origin = PayLoad.Position;
            #endif

                // Attenuate light
                vec3 AlbedoColor = min(Material.AlbedoColor.rgb, vec3(0.99));
                SampleColor = AlbedoColor * SampleColor;
            }
            else if (Material.Type == MATERIAL_EMISSIVE) 
            {
                // Add light
                EmissiveColor    = Material.EmissiveColor.rgb;
                SampleColor = SampleColor * EmissiveColor;

                // EmissiveColor materials do not scatter
                break;
            }
            else
            {
                // Invalid material
                SampleColor = vec3(0.0);
                break;
            }

            // Setup the next Ray
            Ray.Origin    = Origin;
            Ray.Direction = Direction;
        }
        else
        {
            // Add this hit color
            vec3 BackGroundColor = GetEnvironmentLight(Ray.Direction);
            SampleColor = SampleColor * BackGroundColor;
            break;
        }
    }

    vec3 FinalColor = SampleColor;

    // Accumulate samples over time
    vec4 previousColor = imageLoad(uAccumulation, Pixel);
    vec4 currentColor  = previousColor + vec4(FinalColor, 0.0);
    imageStore(uAccumulation, Pixel, currentColor);

    // Store to scene texture
    FinalColor = currentColor.rgb / max(uRandom.NumSamples, 1.0);
    FinalColor = vec3(1.0) - exp(-FinalColor * uScene.Exposure);
    FinalColor = pow(FinalColor, vec3(1.0 / GAMMA));
    imageStore(uOutput, Pixel, vec4(FinalColor, 1.0));
}
#endif