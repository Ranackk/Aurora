#pragma once

#include <glm\ext\vector_float3.hpp>

namespace Math
{
	struct Tetrahedron
	{	
		glm::vec3 PointOrigin = glm::vec3(0.0f);
		glm::vec3 PointB	  = glm::vec3(0.0f);
		glm::vec3 PointC	  = glm::vec3(0.0f);
		glm::vec3 PointD	  = glm::vec3(0.0f);

		//////////////////////////////////////////////////////////////////////////

		Tetrahedron() = default;
		explicit Tetrahedron(const glm::vec3& pointOrigin, const glm::vec3& pointB, const glm::vec3& pointC, const glm::vec3& pointD) :
			PointOrigin(pointOrigin), PointB(pointB), PointC(pointC), PointD(pointC) {}

	};
}