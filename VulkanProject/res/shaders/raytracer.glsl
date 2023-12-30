#version 450
#include "halton.glsl"
#include "random.glsl"
#include "math.glsl"
#include "tonemap.glsl"

#define NUM_THREADS (16)

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

#define MAX_DEPTH   (1)
#define NUM_SAMPLES (1)

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

#define MAT_METAL      (1)
#define MAT_LAMBERTIAN (2)
#define MAT_EMISSIVE   (3)
#define MAT_DIELECTRIC (4)

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
    { vec3( 1.0f, 0.5f, 1.0f), 0.5f, 8 }, 
    { vec3( 0.0f, 0.5f, 1.0f), 0.5f, 1 }, 
    { vec3(-1.0f, 0.5f, 1.0f), 0.5f, 2 }, 
};

#define NUM_PLANES 2
const Plane GPlanes[NUM_PLANES] =
{
    { vec3(0.0f, 1.0f, 0.0),  0.0f, 3 },
    //{ vec3(1.0f, 0.0f, 0.0),  3.0f, 4 },
    //{ vec3(1.0f, 0.0f, 0.0), -3.0f, 5 },
    { vec3(0.0f, 0.0f, 1.0),  3.0f, 6 },
    //{ vec3(0.0f, 1.0f, 0.0),  5.0f, 7 },
};

/* #define NUM_MATERIALS 9
const Material GMaterials[NUM_MATERIALS] =
{
    { MAT_LAMBERTIAN, vec3(1.0f,  0.1f,  0.1f),  0.0f, 0.0f }, // 0
    { MAT_METAL,      vec3(1.0f,  1.0f,  1.0f),  1.0f, 0.0f }, // 1
    { MAT_METAL,      vec3(1.0f,  1.0f,  1.0f),  0.0f, 0.0f }, // 2
    { MAT_LAMBERTIAN, vec3(0.01f, 0.01f, 1.0f),  1.0f, 0.0f }, // 3
    { MAT_LAMBERTIAN, vec3(0.01f, 1.0f,  0.01f), 1.0f, 0.0f }, // 4
    { MAT_LAMBERTIAN, vec3(1.0f,  0.01f, 0.01f), 1.0f, 0.0f }, // 5
    { MAT_LAMBERTIAN, vec3(1.0f,  1.0f,  1.0f),  1.0f, 0.0f }, // 6
    { MAT_EMISSIVE,   vec3(10.0f, 10.0f, 10.0f), 0.0f, 0.0f }, // 7
    { MAT_LAMBERTIAN, vec3(1.0f,  1.0f,  1.0f),  0.0f, 1.5f }, // 8
}; */

vec3 HemisphereSampleUniform(float u, float v) 
{
    float phi = v * 2.0 * PI;
    float cosTheta = 1.0 - u;
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    return normalize(vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta));
}

void HitSphere(in Sphere Sphere, in Ray Ray, inout RayPayLoad PayLoad)
{
    vec3 Distance = Ray.Origin - Sphere.Position;
    float a = dot(Ray.Direction, Ray.Direction);
    float b = dot(Ray.Direction, Distance);
    float c = dot(Distance, Distance) - (Sphere.Radius * Sphere.Radius);

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

                vec3 OutsideNormal = normalize((Position - Sphere.Position) / Sphere.Radius);
                if (dot(Ray.Direction, OutsideNormal) > 0.0f)
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
    float DdotN = dot(Ray.Direction, Plane.Normal);
    if (abs(DdotN) > 0.0001f)
    {
        vec3 Center = Plane.Normal * Plane.Distance;
        vec3 Diff   = Center - Ray.Origin;

        float t = dot(Diff, Plane.Normal) / DdotN;
        if (t > 0)
        {
            if (t < PayLoad.T)
            {
                PayLoad.MaterialIndex = Plane.MaterialIndex;
                PayLoad.FrontFace = true;
                PayLoad.T = t;

                if (DdotN > 0.0f)
                {
                    PayLoad.Normal = -normalize(Plane.Normal);
                }
                else
                {
                    PayLoad.Normal = normalize(Plane.Normal);
                }
            }
        }
    }
}

bool TraceRay(in Ray Ray, inout RayPayLoad PayLoad)
{
    for (uint i = 0; i < NUM_SPHERES; i++)
    {
        Sphere Sphere = GSpheres[i];
        HitSphere(Sphere, Ray, PayLoad);
    }

    /*for (uint i = 0; i < NUM_PLANES; i++)
    {
        Plane Plane = GPlanes[i];
        HitPlane(Plane, Ray, PayLoad);
    }*/

    if (PayLoad.T < PayLoad.MaxT)
    {
        // PayLoad.MaterialIndex = min(PayLoad.MaterialIndex, NUM_MATERIALS - 1);
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
                /*Material Material = GMaterials[PayLoad.MaterialIndex];
                vec3 N = normalize(PayLoad.Normal);

                vec3 Position     = Ray.Origin + Ray.Direction * PayLoad.T;
                vec3 NewOrigin    = Position + (N * 0.0001f);
                vec3 NewDirection = vec3(0.0f);

                if (Material.Type == MAT_LAMBERTIAN)
                {
                    ShadeLambertian(Material, Ray, PayLoad, N, HitColor, NewDirection, RandomSeed);
                }
                else if (Material.Type == MAT_METAL)
                {
                    ShadeMetal(Material, Ray, PayLoad, N, HitColor, NewDirection, RandomSeed);
                }
                else if (Material.Type == MAT_EMISSIVE)
                {
                    ShadeEmissive(Material, Ray, PayLoad, N, HitColor, NewDirection, RandomSeed);
                }
                else if (Material.Type == MAT_DIELECTRIC)
                {
                    ShadeDielectric(Material, Ray, PayLoad, N, HitColor, NewDirection, RandomSeed);
                    NewOrigin = Position;
                }

                Ray.Origin    = NewOrigin;
                Ray.Direction = NewDirection;*/

                HitColor = vec3(1.0f, 0.0f, 0.0f);
            }
            else
            {
                HitColor = vec3(0.0f, 0.0f, 0.0f);
                i = MAX_DEPTH;
            }

            SampleColor = SampleColor * HitColor;
        }

        FinalColor += SampleColor;
    }

    FinalColor = FinalColor / vec3(NUM_SAMPLES);
    // FinalColor = AcesFitted(FinalColor);
    // FinalColor = pow(FinalColor, vec3(1.0f / 2.2f));
    imageStore(Output, Pixel, vec4(FinalColor, 1.0f));
}