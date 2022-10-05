#pragma once

#include <glm\ext\vector_float3.hpp>

namespace Math
{
	struct Triangle
	{	
		glm::vec3 PointOrigin = glm::vec3(0.0f);
		glm::vec3 PointB	  = glm::vec3(0.0f);
		glm::vec3 PointC	  = glm::vec3(0.0f);

		//////////////////////////////////////////////////////////////////////////

		Triangle() = default;
		explicit Triangle(const glm::vec3& pointOrigin, const glm::vec3& pointB, const glm::vec3& pointC) :
			PointOrigin(pointOrigin), PointB(pointB), PointC(pointC) {}

	};
}