#pragma once
#include "Core.h"

struct SCameraBuffer
{
    // 0-64
    glm::mat4 Projection;
    // 64-128
    glm::mat4 View;
    // 128-192
    glm::mat4 InverseProjection;
    // 192-256
    glm::mat4 InverseView;
    // 256-288
    glm::vec4 Position;
    glm::vec4 Forward;
    // 288-292
    float FieldOfViewDegrees;

    // Padding
    uint32_t Padding0;
    uint32_t Padding1;
    uint32_t Padding2;
};

class CCamera
{
public:
    CCamera()
    {
        Reset();
    }
    
    void Move(const glm::vec3& translation)
    {
        m_Position = m_Position + translation.x * m_Right + translation.y * m_Up + m_Forward * translation.z;
    }

    void Rotate(const glm::vec3& rotation)
    {
        m_Rotation.x += rotation.x;
        m_Rotation.x = std::max<float>(glm::radians(-89.0f), std::min<float>(glm::radians(89.0f), m_Rotation.x));

        m_Rotation.y += rotation.y;
        m_Rotation.z += rotation.z;

        glm::mat4 rotationMatrix = glm::eulerAngleYXZ(m_Rotation.y, m_Rotation.x, m_Rotation.z);
        m_Forward = glm::vec3(0.0f, 0.0f, 1.0f);
        m_Forward = glm::normalize(rotationMatrix * glm::vec4(m_Forward, 0.0f));

        m_Up    = glm::vec3(0.0f, 1.0f, 0.0f);
        m_Right = glm::normalize(glm::cross(m_Forward, m_Up));
        m_Up    = glm::normalize(glm::cross(m_Right, m_Forward));
    }

    void Update(float fovDegrees, float width, float height, float near, float far)
    {
        m_FieldOfView = glm::radians(fovDegrees);
        m_View        = glm::lookAtLH(m_Position, m_Position + m_Forward, m_Up);
        m_Projection  = glm::perspectiveFovLH(m_FieldOfView, width, height, near, far);
    }

    void Reset()
    {
        m_Position = glm::vec3(0.0f, 0.0f, -1.0f);
        m_Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        m_Forward  = glm::vec3(0.0f, 0.0f, 1.0f);
        m_Up       = glm::vec3(0.0f, 1.0f, 0.0f);
        m_Right    = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    const glm::mat4& GetViewMatrix() const
    {
        return m_View;
    }
    
    const glm::mat4& GetProjectionMatrix() const
    {
        return m_Projection;
    }
    
    glm::mat4 GetInverseViewMatrix() const
    {
        return glm::inverse(m_View);
    }
    
    glm::mat4 GetInverseProjectionMatrix() const
    {
        return glm::inverse(m_Projection);
    }
    
    glm::mat4 GetMatrix() const
    {
        return m_Projection * m_View;
    }
    
    const glm::vec3& GetPosition() const
    {
        return m_Position;
    }
    
    const glm::vec3& GetForward() const
    {
        return m_Forward;
    }
    
    float GetFieldOfView() const
    {
        return m_FieldOfView;
    }
    
private:
    glm::mat4 m_View = glm::identity<glm::mat4>();
    glm::mat4 m_Projection = glm::identity<glm::mat4>();
    glm::vec3 m_Position;
    glm::vec3 m_Rotation;
    glm::vec3 m_Forward;
    glm::vec3 m_Up;
    glm::vec3 m_Right;
    float     m_FieldOfView = glm::pi<float>() / 2.0f;
};
