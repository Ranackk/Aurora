#pragma once

#include <glm/ext/vector_float2.hpp>

#include "MathLib/Types/Circle.h"

namespace Math
{
	inline bool Intersects(const Circle& circle, const Ray<glm::vec2>& ray)
	{
		return true;
	}
	
	//////////////////////////////////////////////////////////////////////////

	inline bool Contains(const Circle& circle, const glm::vec2& vector2)
	{
		return DistanceSquared(circle.Origin, vector2) <= circle.Radius * circle.Radius;
	}

	//////////////////////////////////////////////////////////////////////////
}