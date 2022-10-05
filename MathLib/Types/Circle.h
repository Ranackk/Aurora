#pragma once

#include <glm/ext/vector_float2.hpp>
namespace Math
{
	struct Circle
	{
		glm::vec2 Origin	= glm::vec2(0.0f);
		float Radius		= 0.0f;

		//////////////////////////////////////////////////////////////////////////

		Circle() = default;
		explicit Circle(const glm::vec2& origin, const float radius) : Origin(origin), Radius(radius) {}

	};

	//////////////////////////////////////////////////////////////////////////
}