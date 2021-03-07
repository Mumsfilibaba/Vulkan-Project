#version 450

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

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
#define NUM_SAMPLES 16

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

#define MAT_METAL     1
#define MAT_DILECTRIC 2
#define MAT_REFRACT   3

struct Material
{
    int   Type;
    vec3  Albedo;
    float Roughness;
};

#define NUM_SPHERES 3
const Sphere gSpheres[NUM_SPHERES] =
{
    { vec3( 1.0f, 0.5f, 2.0f), 0.5f, 0 }, 
    { vec3( 0.0f, 0.5f, 1.0f), 0.5f, 1 }, 
    { vec3(-1.0f, 0.5f, 2.0f), 0.5f, 2 }, 
};

#define NUM_PLANES 1
const Plane gPlanes[NUM_PLANES] = 
{
    { vec3(0.0f, 1.0f, 0.0), 0.0f, 3 }
};

#define NUM_MATERIALS 4
const Material gMaterials[NUM_MATERIALS] = 
{
    { MAT_DILECTRIC, vec3(1.0f, 0.2f, 0.2f), 0.0f },
    { MAT_DILECTRIC, vec3(0.2f, 0.2f, 1.0f), 0.5f },
    { MAT_METAL,     vec3(0.2f, 1.0f, 0.2f), 0.15f },
    { MAT_METAL,     vec3(1.0f, 1.0f, 0.2f), 0.3f },
};

float RadicalInverse(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse(i));
}

const float PI = 3.14159265358979f;

vec3 HemisphereSampleUniform(float u, float v) 
{
    float phi = v * 2.0 * PI;
    float cosTheta = 1.0 - u;
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    return normalize(vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta));
}

float Random(vec3 Seed, int i)
{
    vec4  Seed4 = vec4(Seed, i);
    float Dot   = dot(Seed4, vec4(12.9898f, 78.233f, 45.164f, 94.673f));
    return fract(sin(Dot) * 43758.5453f);
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
        Sphere s = gSpheres[i];
        HitSphere(s, r, PayLoad);
    }

    for (uint i = 0; i < NUM_PLANES; i++)
    {
        Plane p = gPlanes[i];
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

void main()
{
    vec3 CameraPosition = uCamera.Position.xyz;
    vec3 CamForward     = normalize(uCamera.Forward.xyz);
    vec3 CamUp          = vec3(0.0f, 1.0f, 0.0f);
    CamUp = normalize(CamUp - dot(CamUp, CamForward) * CamForward);
    vec3 CamRight = normalize(cross(CamUp, CamForward));

    ivec2 TexCoord  = ivec2(gl_GlobalInvocationID.xy);
    ivec2 Size      = ivec2(gl_NumWorkGroups.xy * gl_WorkGroupSize.xy);
    vec2 FilmCorner = vec2(-1.0f, -1.0f);
    vec2 FilmUV     = vec2(TexCoord) / vec2(Size.xy);
    FilmUV.y = 1.0f - FilmUV.y;
    FilmUV   = FilmUV * 2.0f;

    float AspectRatio = float(Size.x) / float(Size.y);
    vec2  FilmCoord = FilmCorner + FilmUV;
    FilmCoord.x = FilmCoord.x * AspectRatio;

    float FilmDistance = 1.0f;
    vec3 FilmCenter = CameraPosition + (CamForward * FilmDistance);
    vec3 FilmTarget = FilmCenter + (CamRight * FilmCoord.x) + (CamUp * FilmCoord.y);

    vec3 FinalColor = vec3(0.0f);
    for (uint Sample = 0; Sample < NUM_SAMPLES; Sample++)
    {
        Ray r;
        r.Origin    = CameraPosition;
        r.Direction = normalize(FilmTarget - CameraPosition);

        vec3 Color = vec3(1.0f);
        for (uint i = 0; i < MAX_DEPTH; i++)
        {
            RayPayLoad PayLoad;
            PayLoad.MinT = 0.0f;
            PayLoad.MaxT = 100000.0f;
            PayLoad.T    = PayLoad.MaxT;

            if (!TraceRay(r, PayLoad))
            {
                Color = Color * vec3(0.5f, 0.7f, 1.0f);
                break;
            }

            Material Mat = gMaterials[PayLoad.MaterialIndex];
            vec3 Albedo    = Mat.Albedo;
            vec3 N         = normalize(PayLoad.Normal);
            vec3 Position  = r.Origin + r.Direction * PayLoad.T;
            vec3 NewOrigin = Position + (N * 0.00001f);

            if (Mat.Type == MAT_DILECTRIC)
            {
                vec2  Halton = Hammersley((uRandom.FrameIndex + Sample) % 16, 16);
                float Rnd0 = Random(vec3(ivec3(TexCoord.xy, TexCoord.x)), int(uRandom.FrameIndex));
                float Rnd1 = Random(vec3(ivec3(TexCoord.xy, TexCoord.y)), int(uRandom.FrameIndex));
                Halton.x = fract(Halton.x + Rnd0);
                Halton.y = fract(Halton.y + Rnd1);

                vec3 Rnd = HemisphereSampleUniform(Halton.x, Halton.y);
                if (dot(Rnd, N) <= 0.0)
                {
                    Rnd = -Rnd;
                }

                vec3 Target = Position + Rnd;
                vec3 NewDir = normalize(Target - Position);

                Color = Albedo * Color;

                r.Origin    = NewOrigin;
                r.Direction = NewDir;
            }
            else if (Mat.Type == MAT_METAL)
            {
                vec2  Halton = Hammersley((uRandom.FrameIndex + Sample) % 16, 16);
                float Rnd0 = Random(vec3(ivec3(TexCoord.xy, TexCoord.x)), int(uRandom.FrameIndex));
                float Rnd1 = Random(vec3(ivec3(TexCoord.xy, TexCoord.y)), int(uRandom.FrameIndex));
                Halton.x = fract(Halton.x + Rnd0);
                Halton.y = fract(Halton.y + Rnd1);

                vec3 Rnd = HemisphereSampleUniform(Halton.x, Halton.y);
                if (dot(Rnd, N) <= 0.0)
                {
                    Rnd = -Rnd;
                }

                vec3 Reflection = normalize(reflect(r.Direction, N));
                Reflection = normalize(Reflection + Rnd * Mat.Roughness);

                Color = Color * Albedo;
                
                r.Origin    = NewOrigin;
                r.Direction = Reflection;
            }
            else
            {
                Color = Color * vec3(0.0f);
                break;
            }
        }

        FinalColor += Color;
    }

    FinalColor = FinalColor / NUM_SAMPLES;
    FinalColor = pow(FinalColor, vec3(1.0f / 2.2f));
    imageStore(Output, TexCoord, vec4(FinalColor, 1.0f));
}