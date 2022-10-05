#pragma once

#include <assert.h>  

#include <cmath>
#include <corecrt_math_defines.h>
#include <algorithm>
#include <cstdint>

#include <glm\ext\vector_float2.hpp>
#include <glm\ext\vector_float3.hpp>
#include <glm\ext\vector_float4.hpp>
#include <glm\gtx\compatibility.hpp>
#include <Vendor\bitmap_image.hpp>

#include "Rendering/CUDATypes.h"

namespace Math
{
	//////////////////////////////////////////////////////////////////////////
	// DISTANCES
	//////////////////////////////////////////////////////////////////////////

	constexpr inline float DistanceSquared(const glm::vec2& lhs, const glm::vec2& rhs)
	{
		return	(rhs.x - lhs.x) * (rhs.x - lhs.x) +
				(rhs.y - lhs.y) * (rhs.y - lhs.y);
	}

	//////////////////////////////////////////////////////////////////////////

	inline float Distance(const glm::vec2& lhs, const glm::vec2& rhs)
	{
		return std::sqrt(DistanceSquared(lhs, rhs));
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline float LengthSquared(const glm::vec2& val)
	{
		return val.x * val.x + val.y * val.y;
	}

	//////////////////////////////////////////////////////////////////////////

	inline float Length(const glm::vec2& val)
	{
		return std::sqrt(LengthSquared(val));
	}

	//////////////////////////////////////////////////////////////////////////

	constexpr inline float DistanceSquared(const glm::vec3& lhs, const glm::vec3& rhs)
	{
		return	(rhs.x - lhs.x) * (rhs.x - lhs.x) +
				(rhs.y - lhs.y) * (rhs.y - lhs.y) +
				(rhs.z - lhs.z) * (rhs.z - lhs.z);
	}

	//////////////////////////////////////////////////////////////////////////

	inline float Distance(const glm::vec3& lhs, const glm::vec3& rhs)
	{
		return std::sqrt(DistanceSquared(lhs, rhs));
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU constexpr inline float LengthSquared(const glm::vec3& val)
	{
		return	val.x * val.x +
				val.y * val.y +
				val.z * val.z;
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline float Length(const glm::vec3& val)
	{
		return std::sqrt(LengthSquared(val));
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU constexpr inline float DistanceSquared(const glm::vec4& lhs, const glm::vec4& rhs)
	{
		return	(rhs.x - lhs.x) * (rhs.x - lhs.x) +
				(rhs.y - lhs.y) * (rhs.y - lhs.y) +
				(rhs.z - lhs.z) * (rhs.z - lhs.z) +
				(rhs.w - lhs.w) * (rhs.w - lhs.w);
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline float Distance(const glm::vec4& lhs, const glm::vec4& rhs)
	{
		return std::sqrt(DistanceSquared(lhs, rhs));
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU constexpr inline float LengthSquared(const glm::vec4& val)
	{
		return	val.x * val.x +
				val.y * val.y +
				val.z * val.z +
				val.w * val.w;
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline float Length(const glm::vec4& val)
	{
		return std::sqrt(LengthSquared(val));
	}

	//////////////////////////////////////////////////////////////////////////
	// 2D ROTATION
	//////////////////////////////////////////////////////////////////////////

	// Angle in degree
	A_CUDA_CPUGPU inline glm::vec2 RotateAngle(const glm::vec2& vec, float angle)
	{
		return glm::vec2(glm::cos(angle) * vec.x - glm::sin(angle) * vec.y, 
						 glm::sin(angle) * vec.x + glm::cos(angle) * vec.y);
	}
	
	//////////////////////////////////////////////////////////////////////////
	// DOT & CROSS
	//////////////////////////////////////////////////////////////////////////

	constexpr inline float Dot(const glm::vec2& lhs, const glm::vec2& rhs)
	{
		return	lhs.x * rhs.x + 
				lhs.y * rhs.y;
	}

	//////////////////////////////////////////////////////////////////////////

	constexpr inline float Dot(const glm::vec3& lhs, const glm::vec3& rhs)
	{
		return	lhs.x * rhs.x +
				lhs.y * rhs.y +
				lhs.z * rhs.z;
	}

	//////////////////////////////////////////////////////////////////////////

	constexpr inline float Dot(const glm::vec4& lhs, const glm::vec4& rhs)
	{
		return	lhs.x * rhs.x +
				lhs.y * rhs.y +
				lhs.z * rhs.z +
				lhs.w * rhs.w;
	}

	//////////////////////////////////////////////////////////////////////////

	inline glm::vec3 Cross(const glm::vec3& lhs, const glm::vec3& rhs)
	{
		return glm::vec3(	lhs.y * rhs.z - lhs.z * rhs.y,
						lhs.z * rhs.x - lhs.x * rhs.z,
						lhs.x * rhs.y - lhs.y * rhs.x);
	}
	
	//////////////////////////////////////////////////////////////////////////

	inline glm::vec4 TernaryCross(const glm::vec4& a, const glm::vec4& b, const glm::vec4& c)
	{
		// https://www.jstor.org/stable/2688595?read-now=1&seq=5#page_scan_tab_contents // this a good source!
		float x = glm::dot({a.y, a.z, a.w}, glm::cross(glm::vec3(b.y, b.z, b.w), glm::vec3(c.y, c.z, c.w)));
		float y = glm::dot({a.x, a.z, a.w}, glm::cross(glm::vec3(b.x, b.z, b.w), glm::vec3(c.x, c.z, c.w)));
		float z = glm::dot({a.x, a.y, a.w}, glm::cross(glm::vec3(b.x, b.y, b.w), glm::vec3(c.x, c.y, c.w)));
		float w = glm::dot({a.x, a.y, a.z}, glm::cross(glm::vec3(b.x, b.y, b.z), glm::vec3(c.x, c.y, c.z)));
		
		return glm::vec4(x, y, z, w);
	}

	//////////////////////////////////////////////////////////////////////////
	// CLAMP
	//////////////////////////////////////////////////////////////////////////
	
	A_CUDA_CPUGPU constexpr inline int Clamp(const int val, const int min, const int max)
	{
		assert(!(max < min));
		return (val < min) ? min : (val > max) ? max : val;
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU constexpr inline float Clamp(const float val, const float min, const float max)
	{
		assert(!(max < min));
		return (val < min) ? min : (val > max) ? max : val;
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU constexpr inline float Clamp01(const float val)
	{
		return (val < 0.0f) ? 0.0f : (val > 1.0f) ? 1.0f : val;
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline glm::vec2 Clamp(const glm::vec2& val, const glm::vec2& min, const glm::vec2& max)
	{
		return glm::vec2(	Clamp(val.x, min.x, max.x),
							Clamp(val.y, min.y, max.y));
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline glm::vec3 Clamp(const glm::vec3& val, const glm::vec3& min, const glm::vec3& max)
	{
		return glm::vec3(	Clamp(val.x, min.x, max.x),
							Clamp(val.y, min.y, max.y),
							Clamp(val.z, min.z, max.z));
	}

	//////////////////////////////////////////////////////////////////////////
	// ABS
	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline float Abs(const float val)
	{
		return std::abs(val);
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline glm::vec2 Abs(const glm::vec2& val)
	{
		return glm::vec2(Abs(val.x), Abs(val.y));
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline glm::vec3 Abs(const glm::vec3& val)
	{
		return glm::vec3(Abs(val.x), Abs(val.y), Abs(val.z));
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline glm::vec4 Abs(const glm::vec4& val)
	{
		return glm::vec4(Abs(val.x), Abs(val.y), Abs(val.z), Abs(val.w));
	}

	//////////////////////////////////////////////////////////////////////////
	// MIN / MAX
	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline float Min(const float lhs, const float rhs)
	{
		return glm::min(lhs, rhs);
	}
	
	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline glm::vec3 Min(const glm::vec3& lhs, const glm::vec3& rhs)
	{
		return glm::min(lhs, rhs);
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline glm::vec4 Min(const glm::vec4& lhs, const glm::vec4& rhs)
	{
		return glm::min(lhs, rhs);
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline float Max(const float lhs, const float rhs)
	{
		return glm::max(lhs, rhs);
	}
	
	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline glm::vec3 Max(const glm::vec3& lhs, const glm::vec3& rhs)
	{
		return glm::max(lhs, rhs);
	}
	
	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline glm::vec4 Max(const glm::vec4& lhs, const glm::vec4& rhs)
	{
		return glm::max(lhs, rhs);
	}
	
	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline float MaxComponent(const glm::vec3& vec)
	{
		return glm::max(glm::max(vec.x, vec.y), vec.z);
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU inline float MaxComponent(const glm::vec4& vec)
	{
		return glm::max(glm::max(glm::max(vec.x, vec.y), vec.z), vec.w);
	}

	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// LERP
	//////////////////////////////////////////////////////////////////////////
	
	A_CUDA_CPUGPU constexpr inline float UnclampedLerp(const float min, const float max, const float t)
	{
		return t * max + (1.0f - t) * min;
	}
	
	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU constexpr inline float InverseUnclampedLerp(const float min, const float max, const float t)
	{
		return (t - min) / (max - min);
	}

	//////////////////////////////////////////////////////////////////////////

	// TODO: Move out of core!
	A_CUDA_CPUGPU inline glm::vec3 UnclampedLerp(const glm::vec3& a, const glm::vec3& b, const float t)
	{
		return glm::lerp(a, b, t);
	}

	//////////////////////////////////////////////////////////////////////////
	// REMAP
	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU constexpr inline float Remap(const float oldMin, const float oldMax, const float newMin, const float newMax, const float t)
	{
		return newMin + (t - oldMin) * (newMax -  newMin) / (oldMax - oldMin);
	}

	//////////////////////////////////////////////////////////////////////////
	// SMOOTHSTEP
	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU constexpr inline float Smoothstep(const float min, const float max, const float t)
	{
		float step = Clamp01((t - min) / (max - min));
		return step * step * (3 - 2 * step);
	}
	

	//////////////////////////////////////////////////////////////////////////
	// DEG <-> RAD
	//////////////////////////////////////////////////////////////////////////
	
	A_CUDA_CPUGPU constexpr inline float DegToRad(const float deg)
	{
		return static_cast<float>(M_PI * 2.0f / 360.f * deg);
	}

	A_CUDA_CPUGPU constexpr inline float RadToDeg(const float rad)
	{
		return static_cast<float>(rad / (2.0f * M_PI) * 360.0f);
	}

	//////////////////////////////////////////////////////////////////////////
	// Polar
	//////////////////////////////////////////////////////////////////////////

	static glm::vec4 PolarToEuclidean(const glm::vec4& polar)
	{
		return glm::vec4(polar.x * glm::sin(polar.y) * glm::sin(polar.z) * glm::sin(polar.w),
						 polar.x * glm::sin(polar.y) * glm::sin(polar.z) * glm::cos(polar.w),
						 polar.x * glm::sin(polar.y) * glm::cos(polar.z),
						 polar.x * glm::cos(polar.y));
	}

}