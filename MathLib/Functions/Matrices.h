#pragma once
#include <cmath>

#include "Rendering/CUDATypes.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext/matrix_float4x4.hpp>

namespace Math
{
	// Adapted from https://math.stackexchange.com/questions/1402362/rotation-in-4d
	// Note: GLM is column major
	A_CUDA_CPUGPU static inline glm::mat4x4 RotZW(const float a)
	{
		using std::sin;
		using std::cos;

		const float matrix[16] = {
			cos(a),	 sin(a), 0, 0,
			-sin(a), cos(a), 0, 0,
			0,		 	0,	 1, 0,
			0,		 	0,	 0, 1
		};

		return glm::make_mat4x4(matrix);
	}

	//////////////////////////////////////////////////////////////////////////
	
	A_CUDA_CPUGPU static inline glm::mat4x4 RotYW(const float a)
	{
		using std::sin;
		using std::cos;

		const float matrix[16] = {
			cos(a),	 0,	sin(a), 0,
			0,		 1,	0,		0,
			-sin(a), 0,	cos(a),	0,
			0,		 0,	0,		1
		};

		return glm::make_mat4x4(matrix);
	}

	//////////////////////////////////////////////////////////////////////////
	
	A_CUDA_CPUGPU static inline glm::mat4x4 RotYZ(const float a)
	{
		using std::sin;
		using std::cos;

		const float matrix[16] = {
			cos(a),	 0,	0, sin(a),
			0,		 1,	0, 0,		
			0,		 0,	1, 0,		
			-sin(a), 0,	0, cos(a),
		};

		return glm::make_mat4x4(matrix);
	}

	//////////////////////////////////////////////////////////////////////////
	
	A_CUDA_CPUGPU static inline glm::mat4x4 RotXW(const float a)
	{
		using std::sin;
		using std::cos;

		const float matrix[16] = {
			1, 0,		0,		0,
			0, cos(a), sin(a),	0,
			0, -sin(a), cos(a),	0,
			0, 0,		0,		1,
		};

		return glm::make_mat4x4(matrix);
	}

	//////////////////////////////////////////////////////////////////////////
	
	A_CUDA_CPUGPU static inline glm::mat4x4 RotXZ(const float a)
	{
		using std::sin;
		using std::cos;

		const float matrix[16] = {
			1, 0,		0,	0,
			0, cos(a),	0,	sin(a),
			0, 0,		1,	0,
			0, -sin(a),	0,	cos(a)
		};

		return glm::make_mat4x4(matrix);
	}

	//////////////////////////////////////////////////////////////////////////
	
	A_CUDA_CPUGPU static inline glm::mat4x4 RotXY(const float a)
	{
		using std::sin;
		using std::cos;

		const float matrix[16] = {
			1, 0, 0,		0,
			0, 1, 0,		0,
			0, 0, cos(a),	sin(a),
			0, 0, -sin(a),	cos(a)
		};

		return glm::make_mat4x4(matrix);
	}
}
