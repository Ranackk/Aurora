#pragma once

#include "MathLib/Types/Triangle.h"
#include "MathLib/Types/Sphere.h"

#include "MathLib/Functions/Core.h"
#include "MathLib/Constants.h"

namespace Math
{
	inline bool Contains(const Sphere& sphere, const glm::vec3& vector3)
	{
		return DistanceSquared(sphere.Origin, vector3) <= sphere.Radius * sphere.Radius;
	}

	//////////////////////////////////////////////////////////////////////////

	// Möller-Trumbone algorithm.
	// see e.g. https://en.wikipedia.org/wiki/Möller–Trumbore_intersection_algorithm
	inline bool Intersects(const Triangle& triangle, const Ray<glm::vec3>& ray, glm::vec3* outIntersect = nullptr, float* const outTriangleU = nullptr, float* const outTriangleV = nullptr)
	{
		// Triangle is inside a plane defined by
		// Origin + u * edgeA + v * edgeB
		// For u & v between 0 and 1, we are inside the quad that just contains our triangle. 
		// For u + v also < 1, we are inside the correct half of that quad.
		//
		// Ray is defined as Origin + t * Direction.

		const glm::vec3 edgeA = triangle.PointB - triangle.PointOrigin;
		const glm::vec3 edgeB = triangle.PointC - triangle.PointOrigin;

		const glm::vec3 offset	= Cross(ray.Direction, edgeB);
		const float determinant	= Dot(edgeA, offset);

		//////////////////////////////////////////////////////////////////////////

		// We do not intersect if our determinant is smaller than epsilon. In that case we are parallel to the triangle.
		if (determinant >= -Epsilon && determinant <= +Epsilon)
		{
			return false;
		}

		//////////////////////////////////////////////////////////////////////////
		// From here on, the ray intersects the plane (origin + u * dir1 + v * dir2) that contains the triangle. 
		// Now, we need to find out u & v and make sure they are between 0 and 1.

		const float invertedDetermiant	= 1.0f / determinant;
		const glm::vec3 triangleToRay	= ray.Origin - triangle.PointOrigin;

		const float u = invertedDetermiant * Dot(offset, triangleToRay);

		if (u < 0.0f || u > 1.0f)
		{
			return false;
		}

		const glm::vec3 q = Cross(triangleToRay, edgeA);

		const float v = invertedDetermiant * Dot(ray.Direction, q);

		if (v < 0.0f || v > 1.0f)
		{
			return false;
		}

		//////////////////////////////////////////////////////////////////////////
		// From here on, we hit the quad that contains the triangle.

		if (u + v > 1.0f)
		{
			// We hit the other half of the quad
			return false;
		}

		const float t = invertedDetermiant * Dot(edgeB, q);
		if (t < Epsilon)
		{
			// Our ray hits the triangle before its origin. Or right at its origin (which is why we do not check against 0 but Epsilon)
			return false;
		}

		// We hit our triangle with a positive t!!!
		if (outIntersect)
		{
			*outIntersect = ray.Origin + t * ray.Direction;
		}
		
		if (outTriangleU)
		{
			*outTriangleU = u;
		}

		if (outTriangleV)
		{
			*outTriangleV = v;
		}

		return true;
	}
}