#version 450
#include "halton.glsl"
#include "random.glsl"
#include "math.glsl"
#include "tonemap.glsl"

#define NUM_THREADS (16)
#define MAX_DEPTH   (16)

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
    uint NumSamples;
    uint Padding0;
    uint Padding1;
} uRandom;

layout(binding = 4) uniform SceneBufferObject 
{
    vec4 LightDir;
    uint NumSpheres;
    uint NumPlanes;
    uint NumMaterials;
    uint Padding2;
} uScene;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* Scene objects */

#define MATERIAL_STANDARD (1)
#define MATERIAL_EMISSIVE (2)

struct Material
{
    vec4  Albedo;
    vec4  Emissive;

    float Roughness;
    uint  Type;
    uint  Padding1;
    uint  Padding2;
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

layout(std430, binding = 5) buffer SphereBuffer
{
    Sphere Spheres[];
};

layout(std430, binding = 6) buffer PlaneBuffer
{
    Plane Planes[];
};

layout(std430, binding = 7) buffer MaterialBuffer
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
    float phi = v * 2.0 * PI;
    float cosTheta = 1.0 - u;
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    return normalize(vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta));
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
    if (Disc >= 0.0f)
    {
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
}

void HitPlane(in Plane Plane, in Ray Ray, inout RayPayLoad PayLoad)
{
    vec3  PlaneNormal = Plane.NormalAndDistance.xyz;
    float PlaneDist   = Plane.NormalAndDistance.w;

    float DdotN = dot(Ray.Direction, PlaneNormal);
    if (abs(DdotN) > 0.0001)
    {
        vec3 Center = PlaneNormal * PlaneDist;
        vec3 Diff   = Center - Ray.Origin;

        float t = dot(Diff, PlaneNormal) / DdotN;
        if (t > 0)
        {
            if (t < PayLoad.T)
            {
                PayLoad.MaterialIndex = Plane.MaterialIndex;
                PayLoad.FrontFace = true;
                PayLoad.T = t;

                if (DdotN > 0.0f)
                {
                    PayLoad.Normal = -normalize(PlaneNormal);
                }
                else
                {
                    PayLoad.Normal = normalize(PlaneNormal);
                }
            }
        }
    }
}

bool TraceRay(in Ray Ray, inout RayPayLoad PayLoad)
{
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

/*void ShadeLambertian(in Material Material, in Ray Ray, in RayPayLoad PayLoad, in vec3 N, out vec3 Color, out vec3 Direction, inout uint Seed)
{
    vec2 Halton = Halton23(NextRandomInt(Seed) % 16);
    Halton.x = fract(Halton.x + NextRandom(Seed));
    Halton.y = fract(Halton.y + NextRandom(Seed));

    vec3 Rnd = HemisphereSampleUniform(Halton.x, Halton.y);
    if (dot(Rnd, N) <= 0.0f)
    {
        Rnd = -Rnd;
    }

    Direction = normalize(Rnd);
    Color = Material.Albedo;
}

void ShadeMetal(in Material Material, in Ray Ray, in RayPayLoad PayLoad, in vec3 N, out vec3 Color, out vec3 Direction, inout uint Seed)
{
    vec3 Reflection = reflect(Ray.Direction, N);
    
    if (Material.Roughness > 0.0f)
    {
        vec2 Halton = Halton23(NextRandomInt(Seed) % 16);
        Halton.x = fract(Halton.x + NextRandom(Seed));
        Halton.y = fract(Halton.y + NextRandom(Seed));

        vec3 Rnd = HemisphereSampleUniform(Halton.x, Halton.y);
        if (dot(Rnd, N) <= 0.0)
        {
            Rnd = -Rnd;
        }

        Direction = normalize(Reflection + Rnd * Material.Roughness);
    }
    else
    {
        Direction = normalize(Reflection);
    }

    Color = Material.Albedo;
}

void ShadeEmissive(in Material Material, in Ray Ray, in RayPayLoad PayLoad, in vec3 N, out vec3 Color, out vec3 Direction, inout uint Seed)
{
    vec2 Halton = Halton23(NextRandomInt(Seed) % 16);
    Halton.x = fract(Halton.x + NextRandom(Seed));
    Halton.y = fract(Halton.y + NextRandom(Seed));

    vec3 Rnd = HemisphereSampleUniform(Halton.x, Halton.y);
    if (dot(Rnd, N) <= 0.0f)
    {
        Rnd = -Rnd;
    }

    Direction = normalize(Rnd);
    Color = Material.Albedo;
}

void ShadeDielectric(in Material Material, in Ray Ray, in RayPayLoad PayLoad, in vec3 N, out vec3 Color, out vec3 Direction, inout uint Seed)
{
    float RefractionRatio = PayLoad.FrontFace ? 1.0f / Material.IndexOfRefraction : Material.IndexOfRefraction;

    float CosTheta = min(dot(-Ray.Direction, N), 1.0f);
    float SinTheta = sqrt(1.0f - CosTheta*CosTheta);

    vec3 Refracted;
    if (RefractionRatio * SinTheta > 1.0f)
    {
        Refracted = reflect(normalize(Ray.Direction), -normalize(N));
    }
    else
    {
        Refracted = refract(normalize(Ray.Direction), normalize(N), RefractionRatio);
    }

    Direction = normalize(Refracted);
    Color = vec3(1.0f);
}*/

void main()
{
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

    uint RandomSeed = InitRandom(uvec2(Pixel), Size.x, uRandom.FrameIndex);

    vec3 FinalColor = vec3(0.0f);
    for (uint Sample = 0; Sample < 1; Sample++)
    {
        vec2 Jitter = Halton23(RandomSeed);
        Jitter = (Jitter * 2.0) - vec2(1.0);

        vec2 FilmUV = (vec2(Pixel) + Jitter) / vec2(Size.xy);
        FilmUV.y = 1.0 - FilmUV.y;
        FilmUV   = FilmUV * 2.0;

        vec2 FilmCoord = FilmCorner + FilmUV;
        FilmCoord.x = FilmCoord.x * AspectRatio;

        vec3 FilmTarget = FilmCenter + (CamRight * FilmCoord.x) + (CamUp * FilmCoord.y);

        Ray Ray;
        Ray.Origin    = CameraPosition;
        Ray.Direction = normalize(FilmTarget - CameraPosition);

        vec3 SampleColor = vec3(1.0);
        for (uint i = 0; i < MAX_DEPTH; i++)
        {
            RayPayLoad PayLoad;
            PayLoad.MinT = 0.0;
            PayLoad.MaxT = 1000.0;
            PayLoad.T    = PayLoad.MaxT;

            vec3 HitColor = vec3(0.0);
            if (TraceRay(Ray, PayLoad))
            {
                const uint MaterialIndex = min(PayLoad.MaterialIndex, uScene.NumMaterials);
                Material Material = Materials[MaterialIndex];
                
                vec3 N          = normalize(PayLoad.Normal);
                vec3 Position   = Ray.Origin + Ray.Direction * PayLoad.T;
                vec3 Reflection = reflect(Ray.Direction, N);
                
                vec3 Direction;
                if (Material.Roughness > 0.0f)
                {
                    vec2 Halton = Halton23(NextRandomInt(RandomSeed) % 16);
                    Halton.x = fract(Halton.x + NextRandom(RandomSeed));
                    Halton.y = fract(Halton.y + NextRandom(RandomSeed));

                    vec3 Rnd = HemisphereSampleUniform(Halton.x, Halton.y);
                    if (dot(Rnd, N) <= 0.0)
                    {
                        Rnd = -Rnd;
                    }

                    Direction = normalize(Reflection + Rnd * Material.Roughness);
                }
                else
                {
                    Direction = normalize(Reflection);
                }

                // Cast a shadow ray
                const vec3 LightDir = -normalize(uScene.LightDir.xyz);
                Ray.Origin    = Position + (N * 0.01f);
                Ray.Direction = LightDir;

                PayLoad.MinT = 0.0;
                PayLoad.MaxT = 1000.0;
                PayLoad.T    = PayLoad.MaxT;

                float LightIntensity = 1.0;
                if (TraceRay(Ray, PayLoad))
                {
                    LightIntensity = 0.0;
                }
                else
                {
                    LightIntensity = clamp(dot(LightDir, N), 0.0, 1.0);
                }

                HitColor = vec3(0.1, 0.1, 0.1) + Material.Albedo.rgb * LightIntensity;

                // Setup the next ray
                Ray.Origin    = Position + (N * 0.01f);
                Ray.Direction = Direction;
            }
            else
            {
                if (i == 0)
                {
                    HitColor = vec3(0.0, 0.0, 0.0);
                }
                else
                {
                    HitColor = vec3(0.1, 0.1, 1.0);
                }

                i = MAX_DEPTH;
            }

            SampleColor = SampleColor * HitColor;
        }

        FinalColor += SampleColor;
    }

    // Accumulate samples over time
    vec4 previousColor = imageLoad(Accumulation, Pixel);
    vec4 currentColor  = previousColor + vec4(FinalColor, 0.0);
    imageStore(Accumulation, Pixel, currentColor);

    // Store to scene texture
    FinalColor = currentColor.rgb / uRandom.NumSamples;
    // FinalColor = AcesFitted(FinalColor);
    FinalColor = pow(FinalColor, vec3(1.0 / 2.2));
    imageStore(Output, Pixel, vec4(FinalColor, 1.0));
}
