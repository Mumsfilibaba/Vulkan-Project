#pragma once
#include "Core.h"

struct CameraBuffer
{
	glm::mat4 Projection;
	glm::mat4 View;
	glm::vec4 Position;
	glm::vec4 Forward;
};

class Camera
{
public:
	Camera()  = default;
	~Camera() = default;
	
	inline void Move(const glm::vec3& translation)
	{
		m_Position = m_Position + translation.x * m_Right + translation.y * m_Up + m_Forward * translation.z;
	}
	
	inline void Rotate(const glm::vec3& rotation)
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
	
	inline void Update(float fovDegrees, float width, float height, float near, float far)
	{
		m_View 		 = glm::lookAtLH(m_Position, m_Position + m_Forward, m_Up);
		m_Projection = glm::perspectiveFovLH(glm::radians(fovDegrees), width, height, near, far);
	}

	inline const glm::mat4& GetViewMatrix() const
	{
		return m_View;
	}
	
	inline const glm::mat4& GetProjectionMatrix() const
	{
		return m_Projection;
	}
	
	inline glm::mat4 GetMatrix() const
	{
		return m_Projection * m_View;
	}
	
	inline const glm::vec3& GetPosition() const
	{
		return m_Position;
	}
	
	inline const glm::vec3& GetForward() const
	{
		return m_Forward;
	}
	
private:
	glm::mat4 m_View 		= glm::identity<glm::mat4>();
	glm::mat4 m_Projection 	= glm::identity<glm::mat4>();
	glm::vec3 m_Position = glm::vec3(0.0f, 1.0f, -1.25f);
	glm::vec3 m_Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 m_Forward  = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 m_Up		 = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 m_Right    = glm::vec3(1.0f, 0.0f, 0.0f);
	
	float m_FieldOfView = glm::pi<float>() / 2.0f;

};
