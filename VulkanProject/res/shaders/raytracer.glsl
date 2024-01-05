#version 450
#include "halton.glsl"
#include "random.glsl"
#include "math.glsl"
#include "tonemap.glsl"

#define NUM_THREADS (16)
#define MAX_DEPTH   (8)

#define SIGMA (0.000001f)

layout(local_size_x = NUM_THREADS, local_size_y = NUM_THREADS, local_size_z = 1) in;

layout (binding = 0, rgba32f) uniform image2D Output;
layout (binding = 1, rgba32f) uniform image2D Accumulation;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* Global uniforms */

layout(binding = 2) uniform CameraBufferObject 
{
    mat4 Projection;
    mat4 View;
    vec4 Position;
    vec4 Forward;
} uCamera;

layout(binding = 3) uniform RandomBufferObject 
{
    uint FrameIndex;
    uint SampleIndex;
    uint NumSamples;
    uint Padding0;
} uRandom;

layout(binding = 4) uniform SceneBufferObject 
{
    uint NumQuads;
    uint NumSpheres;
    uint NumPlanes;
    uint NumMaterials;
} uScene;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* Scene objects */

#define MATERIAL_LAMBERTIAN (1)
#define MATERIAL_METAL      (2)
#define MATERIAL_EMISSIVE   (3)

struct Material
{
    vec4  Albedo;
    vec4  Emissive;
    float Roughness;
    uint  Type;
    uint  Padding0;
    uint  Padding1;
};

struct Quad
{
    vec4 Position;
    vec4 Edge0;
    vec4 Edge1;
    uint MaterialIndex;
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

struct Sphere
{
    vec4 PositionAndRadius;
    uint MaterialIndex;
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

struct Plane 
{
    vec4 NormalAndDistance;
    uint MaterialIndex;
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

layout(std430, binding = 5) buffer QuadBuffer
{
    Quad Quads[];
};

layout(std430, binding = 6) buffer SphereBuffer
{
    Sphere Spheres[];
};

layout(std430, binding = 7) buffer PlaneBuffer
{
    Plane Planes[];
};

layout(std430, binding = 8) buffer MaterialBuffer
{
    Material Materials[];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* Ray Structs */

struct Ray
{
    vec3 Origin;
    vec3 Direction;
};

struct RayPayLoad
{
    vec3  Normal;
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
    return (Value.x < SIGMA) && (Value.y < SIGMA) && (Value.z < SIGMA); 
}

void HitQuad(in Quad Quad, in Ray Ray, inout RayPayLoad PayLoad)
{
    vec3 Q = Quad.Position.xyz;
    vec3 U = Quad.Edge0.xyz;
    vec3 V = Quad.Edge1.xyz;
    vec3 N = cross(U, V);
    vec3 W = N / dot(N, N);

    vec3  Normal = normalize(N);
    float D = dot(Normal, Q);

    float DdotN = dot(Ray.Direction, Normal);
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

void HitSphere(in Sphere Sphere, in Ray Ray, inout RayPayLoad PayLoad)
{
    vec3  SpherePos    = Sphere.PositionAndRadius.xyz;
    float SphereRadius = Sphere.PositionAndRadius.w;

    vec3 Distance = Ray.Origin - SpherePos;
    float a = dot(Ray.Direction, Ray.Direction);
    float b = dot(Ray.Direction, Distance);
    float c = dot(Distance, Distance) - (SphereRadius * SphereRadius);

    float Disc = (b * b) - (a * c);
    if (Disc < 0.0f)
    {
        return;
    }

    float Root = sqrt(Disc);
    float t = (-b - Root) / a;

    if (PayLoad.MinT < t && t < PayLoad.MaxT)
    {
        if (t < PayLoad.T)
        {
            PayLoad.T = t;

            vec3 Position = Ray.Origin + Ray.Direction * PayLoad.T;
            PayLoad.MaterialIndex = Sphere.MaterialIndex;

            vec3 OutsideNormal = normalize((Position - SpherePos) / SphereRadius);
            if (dot(Ray.Direction, OutsideNormal) >= 0.0)
            {
                PayLoad.Normal    = -OutsideNormal;
                PayLoad.FrontFace = false;
            }
            else
            {
                PayLoad.Normal    = OutsideNormal;
                PayLoad.FrontFace = true;
            }
        }
    }
}

void HitPlane(in Plane Plane, in Ray Ray, inout RayPayLoad PayLoad)
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

bool TraceRay(in Ray Ray, inout RayPayLoad PayLoad)
{
    for (uint i = 0; i < uScene.NumQuads; i++)
    {
        Quad Quad = Quads[i];
        HitQuad(Quad, Ray, PayLoad);
    }

    for (uint i = 0; i < uScene.NumSpheres; i++)
    {
        Sphere Sphere = Spheres[i];
        HitSphere(Sphere, Ray, PayLoad);
    }

    for (uint i = 0; i < uScene.NumPlanes; i++)
    {
        Plane Plane = Planes[i];
        HitPlane(Plane, Ray, PayLoad);
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

void main()
{
    // Setup the camera
    vec3 CameraPosition = uCamera.Position.xyz;
    vec3 CamForward     = normalize(uCamera.Forward.xyz);
    vec3 CamUp          = vec3(0.0, 1.0, 0.0);
    CamUp = normalize(CamUp - dot(CamUp, CamForward) * CamForward);
    vec3 CamRight = normalize(cross(CamUp, CamForward));

    const ivec2 Pixel = ivec2(gl_GlobalInvocationID.xy);
    const ivec2 Size  = ivec2(gl_NumWorkGroups.xy * gl_WorkGroupSize.xy);

    float AspectRatio  = float(Size.x) / float(Size.y);
    vec2  FilmCorner   = vec2(-1.0, -1.0);
    float FilmDistance = 1.0;
    vec3  FilmCenter   = CameraPosition + (CamForward * FilmDistance);

    // Jitter the camera each frame
    uint RandomSeed = InitRandom(uvec2(Pixel), uint(Size.x), uRandom.FrameIndex);

    vec2 Jitter = Halton23(uRandom.SampleIndex);
    Jitter = (Jitter * 2.0) - vec2(1.0);

    vec2 FilmUV = (vec2(Pixel) + Jitter) / vec2(Size.xy);
    FilmUV.y = 1.0 - FilmUV.y;
    FilmUV   = FilmUV * 2.0;

    vec2 FilmCoord = FilmCorner + FilmUV;
    FilmCoord.x = FilmCoord.x * AspectRatio;

    vec3 FilmTarget = FilmCenter + (CamRight * FilmCoord.x) + (CamUp * FilmCoord.y);

    // Setup the first ray
    Ray Ray;
    Ray.Origin    = CameraPosition;
    Ray.Direction = normalize(FilmTarget - CameraPosition);

    // Start tracing rays
    vec3 SampleColor = vec3(1.0);
    for (uint i = 0; i < MAX_DEPTH; i++)
    {
        RayPayLoad PayLoad;
        PayLoad.MinT = 0.0;
        PayLoad.MaxT = 1000.0;
        PayLoad.T    = PayLoad.MaxT;

        if (TraceRay(Ray, PayLoad))
        {
            const uint MaterialIndex = min(PayLoad.MaterialIndex, uScene.NumMaterials);
            Material Material = Materials[MaterialIndex];
            
            vec3 N        = normalize(PayLoad.Normal);
            vec3 Position = Ray.Origin + Ray.Direction * PayLoad.T;
            
            vec3 Emissive  = vec3(0.0);
            vec3 Direction = vec3(0.0);
            if (Material.Type == MATERIAL_LAMBERTIAN)
            {
                vec3 Rnd = NextRandomUnitSphereVec3(RandomSeed);
                Direction = normalize(PayLoad.Normal + Rnd);
                if (IsAlmostZero(Direction))
                {
                    //Direction = PayLoad.Normal;
                }

                // Attenuate light
                vec3 Albedo = Material.Albedo.rgb;
                SampleColor = Albedo * SampleColor;
                // i = MAX_DEPTH;
            }
            else if (Material.Type == MATERIAL_METAL)
            {
                vec3 Rnd = NextRandomHemisphere(RandomSeed, PayLoad.Normal);

                vec3 Reflection = reflect(Ray.Direction, N);
                Direction = normalize(Reflection + Rnd * Material.Roughness);

                // Attenuate light
                vec3 Albedo = Material.Albedo.rgb;
                SampleColor = Albedo * SampleColor;
                                
                //SampleColor = N;
                //i = MAX_DEPTH;
            }
            else if (Material.Type == MATERIAL_EMISSIVE) 
            {
                // Add light
                Emissive    = Material.Emissive.rgb;
                SampleColor = SampleColor + Emissive;

                // Emissive materials do not scatter
                i = MAX_DEPTH;
            }
            else
            {
                // Invalid material
                SampleColor = vec3(0.0);
                i = MAX_DEPTH;
            }

            // Setup the next ray
            Ray.Origin    = Position + (N * SIGMA);
            Ray.Direction = Direction;
        }
        else
        {
            vec3 BackGroundColor;
            
        #if 1
            // Create a gradient
            vec3 UnitDirection = normalize(Ray.Direction);
            float Alpha = 0.5 * (UnitDirection.y + 1.0);
            BackGroundColor = (1.0 - Alpha) * vec3(1.0, 1.0, 1.0) + Alpha * vec3(0.5, 0.7, 1.0);
        #else
            // Only light source is the emissive surfaces
            BackGroundColor = vec3(0.0);
        #endif

            // Break the loop
            i = MAX_DEPTH;

            // Add this hit color
            SampleColor = SampleColor * BackGroundColor;
        }
    }

    vec3 FinalColor = SampleColor;

    // Accumulate samples over time
    vec4 previousColor = imageLoad(Accumulation, Pixel);
    vec4 currentColor  = previousColor + vec4(FinalColor, 0.0);
    imageStore(Accumulation, Pixel, currentColor);

    // Store to scene texture
    FinalColor = currentColor.rgb / uRandom.NumSamples;
    //FinalColor = AcesFitted(FinalColor);
    FinalColor = pow(FinalColor, vec3(1.0 / 2.2));
    imageStore(Output, Pixel, vec4(FinalColor, 1.0));
}
