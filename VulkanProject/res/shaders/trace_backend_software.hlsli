#ifndef TRACE_BACKEND_SOFTWARE_HLSLI
#define TRACE_BACKEND_SOFTWARE_HLSLI

bool BackendTraceRay(SRayDesc RayInput, out STraceHit Hit, inout int2 Stats)
{
    SRay Ray;
    Ray.Origin       = RayInput.Origin;
    Ray.Direction    = RayInput.Direction;
    Ray.InvDirection = 1.0 / Ray.Direction;

    SRayPayload Payload;
    Payload.MinT        = RayInput.MinT;
    Payload.MaxT        = RayInput.MaxT;
    Payload.T           = Payload.MaxT;
    Payload.FrontFace  = 0;
    Payload.FromInside = 0;

    if (!TraceRay(Ray, Payload, Stats))
    {
        Hit.MissEmissive = GetEnvironmentLight(Scene.BackgroundType, Scene.GradientLightStrength, SkyboxTexture, SkyboxSampler, Ray.Direction, SKYBOX_MULTIPLIER);
        return false;
    }

    Hit.Normal        = Payload.Normal;
    Hit.Tangent       = Payload.Tangent;
    Hit.Position      = Payload.Position;
    Hit.Barycentrics  = Payload.Barycentrics;
    Hit.TexCoords     = Payload.TexCoords;
    Hit.MaterialIndex = Payload.MaterialIndex;
    Hit.HitT          = Payload.T;
    Hit.FromInside   = Payload.FromInside;
    return true;
}

SMaterial BackendGetMaterial(uint MaterialIndex)
{
    return Materials[MaterialIndex];
}

uint BackendGetNumMaterials()
{
    return Scene.NumMaterials;
}

#endif


