#pragma once
#include "Core.h"

class Camera
{
public:
	Camera() = default;
	~Camera() = default;
	
	inline void Update()
	{
		m_View = glm::lookAtLH(glm::vec3(0.0f, 0.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		//m_Projection = glm::perspectiveFovLH();
	}

	inline glm::mat4 GetMatrix() const
	{
		return m_Projection * m_View;
	}
	
private:
	glm::mat4 m_View 		= glm::identity<glm::mat4>();
	glm::mat4 m_Projection 	= glm::identity<glm::mat4>();
	
	float m_FieldOfView = glm::pi<float>() / 2.0f;

};
