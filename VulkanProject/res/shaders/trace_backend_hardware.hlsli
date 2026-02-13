#ifndef TRACE_BACKEND_HARDWARE_HLSLI
#define TRACE_BACKEND_HARDWARE_HLSLI

bool BackendTraceRay(SRayDesc RayInput, out STraceHit Hit, inout int2 Stats)
{
    RayDesc TraceDesc;
    TraceDesc.Origin    = RayInput.Origin;
    TraceDesc.Direction = RayInput.Direction;
    TraceDesc.TMin      = RayInput.MinT;
    TraceDesc.TMax      = RayInput.MaxT;

    SRayPayload RayPayload;
    RayPayload.HitNormal        = float3(0.0, 0.0, 0.0);
    RayPayload.HitTangent       = float3(0.0, 0.0, 0.0);
    RayPayload.HitPosition      = float3(0.0, 0.0, 0.0);
    RayPayload.HitBarycentrics  = float3(0.0, 0.0, 0.0);
    RayPayload.HitTexCoord      = float2(0.0, 0.0);
    RayPayload.MissEmissive     = float3(0.0, 0.0, 0.0);
    RayPayload.HitMaterialIndex = 0xFFFFFFFFu;
    RayPayload.HitT             = -1.0;
    RayPayload.FromInside      = 0;

    TraceRay(AccelerationStructure, HW_RAY_FLAGS, 0xff, 0, 0, 0, TraceDesc, RayPayload);

    if (RayPayload.HitMaterialIndex == 0xFFFFFFFFu)
    {
        Hit.MissEmissive = RayPayload.MissEmissive;
        return false;
    }

    Hit.Normal        = RayPayload.HitNormal;
    Hit.Tangent       = RayPayload.HitTangent;
    Hit.Position      = RayPayload.HitPosition;
    Hit.Barycentrics  = RayPayload.HitBarycentrics;
    Hit.TexCoords     = RayPayload.HitTexCoord;
    Hit.MaterialIndex = RayPayload.HitMaterialIndex;
    Hit.HitT          = RayPayload.HitT;
    Hit.FromInside   = RayPayload.FromInside;
    Hit.MissEmissive  = RayPayload.MissEmissive;
    return true;
}

SMaterial BackendGetMaterial(uint MaterialIndex)
{
    return MaterialBuffer[MaterialIndex];
}

uint BackendGetNumMaterials()
{
    return Scene.Settings.NumMaterials;
}

#endif


