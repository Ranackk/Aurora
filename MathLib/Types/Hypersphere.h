#pragma once

#include <glm\ext\vector_float4.hpp>

namespace Math
{
	struct Hypershere
	{
		glm::vec4 Origin	= glm::vec4(0.0f);
		float Radius		= 0.0f;

		//////////////////////////////////////////////////////////////////////////

		Hypershere() = default;
		explicit Hypershere(const glm::vec4& origin, const float radius) : Origin(origin), Radius(radius) {}
	};
}