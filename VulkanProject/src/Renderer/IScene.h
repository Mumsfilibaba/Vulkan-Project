#pragma once

class CCamera;

struct IScene
{
    virtual void Initialize() = 0;
    virtual void Reset() = 0;

    virtual void OnRenderUI() = 0;

    virtual float GetCameraSpeed() const = 0;
    virtual float GetFieldOfView() const = 0;
    virtual float GetExposure() const = 0;

    virtual CCamera& GetCamera() = 0;
    virtual const CCamera& GetCamera() const = 0;
};