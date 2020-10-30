#pragma once
#include "Core.h"

class Camera
{
public:
	Camera() = default;
	~Camera() = default;
	
	glm::mat4 GetMatrix() const
	{
		return m_Projection * m_View;
	}
	
private:
	glm::mat4 m_View 		= glm::identity<glm::mat4>();
	glm::mat4 m_Projection 	= glm::identity<glm::mat4>();
};
