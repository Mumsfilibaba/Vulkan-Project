#version 450
#include "halton.glsl"
#include "random.glsl"
#include "math.glsl"
#include "tonemap.glsl"

#define NUM_THREADS 16

layout(local_size_x = NUM_THREADS, local_size_y = NUM_THREADS, local_size_z = 1) in;

layout (binding = 0, rgba16f) uniform image2D Output;

layout(binding = 1) uniform CameraBufferObject 
{
    mat4 Projection;
    mat4 View;
    vec4 Position;
    vec4 Forward;
} uCamera;

layout(binding = 2) uniform RandomBufferObject 
{
    uint FrameIndex;
    uint Padding0;
    uint Padding1;
    uint Padding2;
} uRandom;

#define MAX_DEPTH   4
#define NUM_SAMPLES 256

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
    int   MaterialIndex;
};

struct Sphere
{
    vec3  Position;
    float Radius;
    int   MaterialIndex;
};

struct Plane 
{
    vec3  Normal;
    float Distance;
    int   MaterialIndex;
};

#define MAT_METAL      1
#define MAT_LAMBERTIAN 2
#define MAT_EMISSIVE   3
#define MAT_DIELECTRIC 4

struct Material
{
    int Type;

    vec3  Albedo;
    float Roughness;

    float IndexOfRefraction;
};

#define NUM_SPHERES 3
const Sphere GSpheres[NUM_SPHERES] =
{
    { vec3( 1.0f, 0.5f, 2.0f), 0.5f, 8 }, 
    { vec3( 0.0f, 0.5f, 1.0f), 0.5f, 1 }, 
    { vec3(-1.0f, 0.5f, 2.0f), 0.5f, 2 }, 
};

#define NUM_PLANES 5
const Plane GPlanes[NUM_PLANES] =
{
    { vec3(0.0f, 1.0f, 0.0),  0.0f, 3 },
    { vec3(1.0f, 0.0f, 0.0),  2.0f, 4 },
    { vec3(1.0f, 0.0f, 0.0), -2.0f, 5 },
    { vec3(0.0f, 0.0f, 1.0),  3.0f, 6 },
    { vec3(0.0f, 1.0f, 0.0),  3.0f, 7 },
};

#define NUM_MATERIALS 9
const Material GMaterials[NUM_MATERIALS] =
{
    { MAT_LAMBERTIAN, vec3(1.0f, 0.1f, 0.1f), 0.0f, 0.0f },
    { MAT_LAMBERTIAN, vec3(0.1f, 0.1f, 1.0f), 0.0f, 0.0f },
    { MAT_METAL,      vec3(1.0f, 1.0f, 1.0f), 0.0f, 0.0f },
    { MAT_LAMBERTIAN, vec3(1.0f, 1.0f, 1.0f), 1.0f, 0.0f },
    { MAT_LAMBERTIAN, vec3(0.1f, 1.0f, 0.1f), 1.0f, 0.0f },
    { MAT_LAMBERTIAN, vec3(1.0f, 0.1f, 0.1f), 1.0f, 0.0f },
    { MAT_LAMBERTIAN, vec3(1.0f, 1.0f, 1.0f), 1.0f, 0.0f },
    { MAT_EMISSIVE,   vec3(1.0f, 1.0f, 1.0f), 1.0f, 0.0f },
    { MAT_DIELECTRIC, vec3(1.0f, 1.0f, 1.0f), 1.0f, 0.0f },
};

vec3 HemisphereSampleUniform(float u, float v) 
{
    float phi = v * 2.0 * PI;
    float cosTheta = 1.0 - u;
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    return normalize(vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta));
}

void HitSphere(in Sphere s, in Ray r, inout RayPayLoad PayLoad)
{
    vec3 Distance = r.Origin - s.Position;
    float a = dot(r.Direction, r.Direction);
    float b = dot(r.Direction, Distance);
    float c = dot(Distance, Distance) - (s.Radius * s.Radius);

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

                vec3 Position = r.Origin + r.Direction * PayLoad.T;
                PayLoad.MaterialIndex = s.MaterialIndex;

                vec3 OutsideNormal = normalize((Position - s.Position) / s.Radius);
                if (dot(r.Direction, OutsideNormal) > 0.0f)
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

void HitPlane(in Plane p, in Ray r, inout RayPayLoad PayLoad)
{
    float DdotN = dot(r.Direction, p.Normal);
    if (abs(DdotN) > 0.0001f)
    {
        vec3 Center = p.Normal * p.Distance;
        vec3 Diff   = Center - r.Origin;

        float t = dot(Diff, p.Normal) / DdotN;
        if (t > 0)
        {
            if (t < PayLoad.T)
            {
                PayLoad.T      = t;
                PayLoad.Normal = normalize(p.Normal);
                PayLoad.MaterialIndex = p.MaterialIndex;
            }
        }
    }
}

bool TraceRay(in Ray r, inout RayPayLoad PayLoad)
{
    for (uint i = 0; i < NUM_SPHERES; i++)
    {
        Sphere s = GSpheres[i];
        HitSphere(s, r, PayLoad);
    }

    for (uint i = 0; i < NUM_PLANES; i++)
    {
        Plane p = GPlanes[i];
        HitPlane(p, r, PayLoad);
    }

    if (PayLoad.T < PayLoad.MaxT)
    {
        PayLoad.MaterialIndex = min(PayLoad.MaterialIndex, NUM_MATERIALS - 1);
        return true;
    }
    else
    {
        return false;
    }
}

void ShadeLambertian(in Material Material, in Ray Ray, in vec3 N, out vec3 Color, out vec3 Direction, inout uint Seed)
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

void ShadeMetal(in Material Material, in Ray Ray, in vec3 N, out vec3 Color, out vec3 Direction, inout uint Seed)
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

void ShadeEmissive(in Material Material, in Ray Ray, in vec3 N, out vec3 Color, out vec3 Direction, inout uint Seed)
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
    Color = 10.0f * Material.Albedo + Material.Albedo;
}

void ShadeDielectric(in Material Material, in Ray Ray, in vec3 N, out vec3 Color, out vec3 Direction, inout uint Seed)
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

void main()
{
    vec3 CameraPosition = uCamera.Position.xyz;
    vec3 CamForward     = normalize(uCamera.Forward.xyz);
    vec3 CamUp          = vec3(0.0f, 1.0f, 0.0f);
    CamUp = normalize(CamUp - dot(CamUp, CamForward) * CamForward);
    vec3 CamRight = normalize(cross(CamUp, CamForward));

    const ivec2 Pixel = ivec2(gl_GlobalInvocationID.xy);
    const ivec2 Size  = ivec2(gl_NumWorkGroups.xy * gl_WorkGroupSize.xy);

    float AspectRatio  = float(Size.x) / float(Size.y);
    vec2  FilmCorner   = vec2(-1.0f, -1.0f);
    float FilmDistance = 1.0f;
    vec3  FilmCenter   = CameraPosition + (CamForward * FilmDistance);

    uint RandomSeed = InitRandom(uvec2(Pixel), Size.x, uRandom.FrameIndex);

    vec3 FinalColor = vec3(0.0f);
    for (uint Sample = 0; Sample < NUM_SAMPLES; Sample++)
    {
        vec2 Jitter = Halton23(Sample);
        Jitter = (Jitter * 2.0f) - vec2(1.0f);

        vec2 FilmUV = (vec2(Pixel) + Jitter) / vec2(Size.xy);
        FilmUV.y = 1.0f - FilmUV.y;
        FilmUV   = FilmUV * 2.0f;

        vec2 FilmCoord = FilmCorner + FilmUV;
        FilmCoord.x = FilmCoord.x * AspectRatio;

        vec3 FilmTarget = FilmCenter + (CamRight * FilmCoord.x) + (CamUp * FilmCoord.y);

        Ray Ray;
        Ray.Origin    = CameraPosition;
        Ray.Direction = normalize(FilmTarget - CameraPosition);

        vec3 SampleColor = vec3(1.0f);
        for (uint i = 0; i < MAX_DEPTH; i++)
        {
            RayPayLoad PayLoad;
            PayLoad.MinT = 0.0f;
            PayLoad.MaxT = 1000.0f;
            PayLoad.T    = PayLoad.MaxT;

            vec3 HitColor = vec3(0.0f);
            if (TraceRay(Ray, PayLoad))
            {
                Material Material = GMaterials[PayLoad.MaterialIndex];
                vec3 N = normalize(PayLoad.Normal);

                vec3 Position  = Ray.Origin + Ray.Direction * PayLoad.T;
                vec3 NewOrigin = Position + (N * 0.00001f);
                vec3 NewDirection = normalize(reflect(Ray.Direction, N));

                if (Material.Type == MAT_LAMBERTIAN)
                {
                    ShadeLambertian(Material, Ray, N, HitColor, NewDirection, RandomSeed);
                }
                else if (Material.Type == MAT_METAL)
                {
                    ShadeMetal(Material, Ray, N, HitColor, NewDirection, RandomSeed);
                }
                else if (Material.Type == MAT_EMISSIVE)
                {
                    ShadeEmissive(Material, Ray, N, HitColor, NewDirection, RandomSeed);
                }
                else if (Material.Type == MAT_DIELECTRIC)
                {
                    ShadeDielectric(Material, Ray, N, HitColor, NewDirection, RandomSeed);
                }

                Ray.Origin    = NewOrigin;
                Ray.Direction = NewDirection;
            }
            else
            {
                HitColor = vec3(0.5f, 0.7f, 1.0f);
                i = MAX_DEPTH;
            }

            SampleColor = SampleColor * HitColor;
        }

        FinalColor += SampleColor;
    }

    FinalColor = FinalColor / vec3(NUM_SAMPLES);
    FinalColor = ReinhardSimple(FinalColor, 1.0f);
    FinalColor = pow(FinalColor, vec3(1.0f / 2.2f));
    imageStore(Output, Pixel, vec4(FinalColor, 1.0f));
}
