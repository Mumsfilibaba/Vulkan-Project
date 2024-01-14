#version 450
#include "halton.glsl"
#include "random.glsl"
#include "math.glsl"
#include "tonemap.glsl"

#define BACKGROUND_TYPE_NONE (0)
#define BACKGROUND_TYPE_GRADIENT (1)
#define BACKGROUND_TYPE_SKYBOX (2)

#define NUM_THREADS (16)
#define MAX_DEPTH   (1024)

#define SIGMA (0.0001)

#define GAMMA (2.2)

#define USE_RAY_OFFSET (0)

layout(local_size_x = NUM_THREADS, local_size_y = NUM_THREADS, local_size_z = 1) in;

layout (binding = 0, rgba32f) uniform image2D uOutput;
layout (binding = 1, rgba32f) uniform image2D uAccumulation;

layout (binding = 9) uniform samplerCube uSkybox;

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
    uint  NumQuads;
    uint  NumSpheres;
    uint  NumPlanes;
    uint  NumMaterials;
    
    uint  BackgroundType;
    float Exposure;
    uint  Padding0;
    uint  Padding1;
} uScene;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
/* Scene objects */

#define MATERIAL_LAMBERTIAN (1)
#define MATERIAL_METAL      (2)
#define MATERIAL_EMISSIVE   (3)
#define MATERIAL_DIELECTRIC (4)

struct Material
{
    vec4  Albedo;
    vec4  Emissive;
    uint  Type;
    float Roughness;
    float RefractionIndex;
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

void HitSphere(in Sphere Sphere, in Ray Ray, inout RayPayLoad PayLoad)
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
        PayLoad.MinT = 0.001;
        PayLoad.MaxT = 1000.0;
        PayLoad.T    = PayLoad.MaxT;

        if (TraceRay(Ray, PayLoad))
        {
            const uint MaterialIndex = min(PayLoad.MaterialIndex, uScene.NumMaterials);
            Material Material = Materials[MaterialIndex];
            
            vec3 N        = normalize(PayLoad.Normal);            
            vec3 Emissive  = vec3(0.0);
            vec3 Origin    = vec3(0.0);
            vec3 Direction = vec3(0.0);

            if (Material.Type == MATERIAL_LAMBERTIAN)
            {
                vec3 Rnd = NextRandomUnitSphereVec3(RandomSeed);
                Direction = normalize(PayLoad.Normal + Rnd);

            #if USE_RAY_OFFSET
                Origin = PayLoad.Position + (N * SIGMA);
            #else
                Origin = PayLoad.Position;
            #endif

                /*if (IsAlmostZero(Direction))
                {
                    Direction = PayLoad.Normal;
                }*/

                // Attenuate light
                vec3 Albedo = Material.Albedo.rgb;
                SampleColor = Albedo * SampleColor;
            }
            else if (Material.Type == MATERIAL_METAL)
            {
                vec3 Rnd = NextRandomHemisphere(RandomSeed, PayLoad.Normal);

                vec3 Reflection = reflect(Ray.Direction, N);
                Direction = normalize(Reflection + Rnd * Material.Roughness);
            #if USE_RAY_OFFSET
                Origin = PayLoad.Position + (N * SIGMA);
            #else
                Origin = PayLoad.Position;
            #endif

                // Attenuate light
                vec3 Albedo = Material.Albedo.rgb;
                SampleColor = Albedo * SampleColor;
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
                
            #if USE_RAY_OFFSET
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
                vec3 Albedo = Material.Albedo.rgb;
                SampleColor = Albedo * SampleColor;
            }
            else if (Material.Type == MATERIAL_EMISSIVE) 
            {
                // Add light
                Emissive    = Material.Emissive.rgb;
                SampleColor = SampleColor * Emissive;

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
            Ray.Origin    = Origin;
            Ray.Direction = Direction;
        }
        else
        {
            vec3 BackGroundColor;
            if (uScene.BackgroundType == BACKGROUND_TYPE_NONE)
            {
                // Only light source is the emissive surfaces
                BackGroundColor = vec3(0.0);
            }
            else if (uScene.BackgroundType == BACKGROUND_TYPE_GRADIENT)
            {
                // Create a gradient
                vec3  UnitDirection = normalize(Ray.Direction);
                float Alpha = 0.5 * (UnitDirection.y + 1.0);
                BackGroundColor = (1.0 - Alpha) * vec3(1.0, 1.0, 1.0) + Alpha * vec3(0.5, 0.7, 1.0);
            }
            else if (uScene.BackgroundType == BACKGROUND_TYPE_SKYBOX)
            {
                // Sample the Skybox
                vec3 UnitDirection = normalize(Ray.Direction);
                vec4 SkyboxColor   = texture(uSkybox, UnitDirection);
                BackGroundColor = SkyboxColor.rgb;
            }

            // Break the loop
            i = MAX_DEPTH;

            // Add this hit color
            SampleColor = SampleColor * BackGroundColor;
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
