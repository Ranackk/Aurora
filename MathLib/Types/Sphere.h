#pragma once

#include <glm\ext\vector_float3.hpp>

namespace Math
{
	struct Sphere
	{
		glm::vec3 Origin	= glm::vec3(0.0f);
		float	Radius		= 0.0f;

		//////////////////////////////////////////////////////////////////////////

		Sphere() = default;
		explicit Sphere(glm::vec3 origin, float radius) : Origin(origin), Radius(radius) {}

	};
}