#version 450

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout (binding = 0, rgba16f) uniform image2D Output;

struct Ray
{
    vec3 Origin;
    vec3 Direction;
};

struct RayPayLoad
{
    vec3  Color;
    vec3  Normal;
    float T;
    float MinT;
    float MaxT;
    bool  FrontFace;
};

struct Sphere
{
    vec3  Position;
    float Radius;
};

struct Plane 
{

};

#define NUM_SPHERES 3
const Sphere gSpheres[NUM_SPHERES] =
{
    { vec3( 1.0f, 0.0f, 2.0f), 0.5f }, 
    { vec3( 0.0f, 0.0f, 2.0f), 0.5f }, 
    { vec3(-1.0f, 0.0f, 2.0f), 0.5f }, 
};

void HitSphere(in Sphere s, in Ray r, inout RayPayLoad PayLoad)
{
    vec3 Distance = r.Origin - s.Position;
    float a = dot(r.Direction, r.Direction);
    float b = dot(r.Direction, Distance);
    float c = dot(Distance, Distance) - (s.Radius * s.Radius);

    float Disc = (b * b) - (a * c);
    if (Disc > 0.0f)
    {
        float Root = sqrt(Disc);
        float t0   = (-b + Root) / a;
        float t1   = (-b - Root) / a;
        float MinT = min(max(t0, 0.0f), max(t1, 0.0f));

        if (MinT > 0.0f && MinT < PayLoad.T)
        {
            PayLoad.T = t0;

            vec3 Position     = r.Origin + r.Direction * t0;
            PayLoad.Normal    = normalize((Position - s.Position) / s.Radius);
            PayLoad.FrontFace = (dot(r.Direction, PayLoad.Normal) < 0.0f);
        }
    }
}

void OnHit(in Ray r, inout RayPayLoad PayLoad)
{
    PayLoad.Color = vec3(1.0f, 0.0f, 0.0f);
}

void OnMiss(in Ray r, inout RayPayLoad PayLoad)
{
    PayLoad.Color = vec3(0.5f, 0.7f, 1.0f);
}

void TraceRay(in Ray r, inout RayPayLoad PayLoad)
{
    for (uint i = 0; i < NUM_SPHERES; i++)
    {
        Sphere s = gSpheres[i];
        HitSphere(s, r, PayLoad);
    }

    if (PayLoad.T < PayLoad.MaxT)
    {
        OnHit(r, PayLoad);
    }
    else
    {
        OnMiss(r, PayLoad);
    }
}

void main()
{
    vec3 CameraPosition = vec3(0.0f, 0.0f, -2.0f);
    vec3 CamForward = vec3(0.0f, 0.0f, 1.0f);
    vec3 CamUp      = vec3(0.0f, 1.0f, 0.0f);
    CamUp = normalize(CamUp - dot(CamUp, CamForward) * CamForward);
    vec3 CamRight = normalize(cross(CamUp, CamForward));

    ivec2 TexCoord  = ivec2(gl_GlobalInvocationID.xy);
    ivec2 Size      = ivec2(gl_NumWorkGroups.xy * gl_WorkGroupSize.xy);
    vec2 FilmCorner = vec2(-1.0f, -1.0f);
    vec2 FilmUV     = (vec2(TexCoord) / vec2(Size.xy)) * 2.0f;

    float AspectRatio = float(Size.x) / float(Size.y);
    vec2  FilmCoord = FilmCorner + FilmUV;
    FilmCoord.x = FilmCoord.x * AspectRatio;

    float FilmDistance = 1.0f;
    vec3  FilmTarget   = CamRight * FilmCoord.x + CamUp * FilmCoord.y + CamForward * FilmDistance;

    Ray FirstRay;
    FirstRay.Origin    = CameraPosition;
    FirstRay.Direction = normalize(FilmTarget - CameraPosition);

    RayPayLoad PayLoad;
    PayLoad.MinT = 0.0f;
    PayLoad.MaxT = 100000.0f;
    PayLoad.T    = PayLoad.MaxT;
    TraceRay(FirstRay, PayLoad);

    imageStore(Output, TexCoord, vec4(PayLoad.Color, 1.0f));
}