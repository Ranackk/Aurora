#pragma once

#include <cuda_runtime_api.h>

#include <memory>

#include "MathLib/SignedDistanceFields/SignedDistanceFieldTypes.h"

#include "Rendering/CUDATypes.h"

#pragma warning( disable : 4984 )

namespace Math
{
	class SignedDistanceField : public CUDAManaged
	{
	public:

		SignedDistanceField() = default;
		~SignedDistanceField() = default;

		// We keep track of one optional listener who has a pointer to us. If we get moved, we update that pointer!
		// As CUDA does not support vtables working with both CPU and GPU, we use a void* pointing to us and all derived classes.
		void** m_Listener = nullptr;
		SignedDistanceField(SignedDistanceField&& other) noexcept
		{
			if (other.m_Listener != nullptr)
			{
				*other.m_Listener = this;
			}
			m_Listener = other.m_Listener;
		}
		SignedDistanceField(const SignedDistanceField& other) noexcept = default;
		SignedDistanceField& operator=(SignedDistanceField&& other) noexcept = default;
		SignedDistanceField& operator=(const SignedDistanceField& other) noexcept = default;
	};

	template <class SDF, class N>
	A_CUDA_CPUGPU static N SampleNormal(const SDF& sdf, const N& position)
	{
		// Base technique discussed at http://iquilezles.org/www/articles/normalsSDF/normalsSDF.htm
		if constexpr (std::is_same<N, glm::vec3>::value)
		{

			constexpr float H	= 0.001f;
			const glm::vec3 a	= {+1, -1, -1};
			const glm::vec3 b	= {-1, -1, +1};
			const glm::vec3 c	= {-1, +1, -1};
			const glm::vec3 d	= {+1, +1, +1};

			return glm::normalize(a * sdf.EvaluateDistance(position + a * H) + 
								  b * sdf.EvaluateDistance(position + b * H) + 
								  c * sdf.EvaluateDistance(position + c * H) + 
								  d * sdf.EvaluateDistance(position + d * H));
		}
		else if constexpr (std::is_same<N, glm::vec4>::value)
		{
			constexpr float H	= 0.015f;
			
			const glm::vec4 a = glm::vec4(0.250000f, 0.322749f, 0.456435f, -0.790569f);	// = glm::normalize(glm::vec4(1 / sqrt(10.0f),		 1 / sqrt(6.0f),  1 / sqrt(3.0f),	-1));
			const glm::vec4 b = glm::vec4(0.250000f, 0.322749f, 0.456435f, 0.790569f);	// = glm::normalize(glm::vec4(1 / sqrt(10.0f),		 1 / sqrt(6.0f),  1 / sqrt(3.0f),	1));
			const glm::vec4 c = glm::vec4(0.250000f, 0.322749f, -0.912871f, 0.000000f);	// = glm::normalize(glm::vec4(1 / sqrt(10.0f),		 1 / sqrt(6.0f),  -2 / sqrt(3.0f),	0));
			const glm::vec4 d = glm::vec4(0.264135f, -0.964486f, 0.000000f, 0.000000f);	// = glm::normalize(glm::vec4(1 / sqrt(10.0f),		 -2 / sqrt(3.0f), 0,				0));
			const glm::vec4 e = glm::vec4(-1.000000f, 0.000000f, 0.000000f, 0.000000f);	// = glm::normalize(glm::vec4(-2 / sqrt(2.0f / 5.0f), 0,			, 0,				0));

			return glm::normalize(a * sdf.EvaluateDistance(position + a * H) + 
								  b * sdf.EvaluateDistance(position + b * H) + 
								  c * sdf.EvaluateDistance(position + c * H) + 
								  d * sdf.EvaluateDistance(position + d * H) + 
								  e * sdf.EvaluateDistance(position + e * H));

		}

		// (Currently) Unsupported Vectortype
		// Can not throw an error in device code.
		return N();
	}

	//////////////////////////////////////////////////////////////////////////

	template <class SDF, class N>
	A_CUDA_CPUGPU static N EvaluateToSurfaceVectorZW(const SDF& sdf, const N& position, float& outDistance)
	{
		// We use a triangle technique to assure that x & y are 0, because otherwise, they can cause issues in our birayToWorldSpace transformations.
		
		constexpr float H	= 0.005f;
			
		const glm::vec4 a = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
		const glm::vec4 b = glm::vec4(0.0f, 0.0f, -0.479f, 0.86f);
		const glm::vec4 c = glm::vec4(0.0f, 0.0f, -0.479f, -0.86f);
			
		outDistance	= sdf.EvaluateDistance(position);

		return -glm::normalize(	a * (sdf.EvaluateDistance(position + a * H) - outDistance) + 
								b * (sdf.EvaluateDistance(position + b * H) - outDistance) + 
								c * (sdf.EvaluateDistance(position + c * H) - outDistance));
	}

	//////////////////////////////////////////////////////////////////////////

	template <class SDF, class N>
	A_CUDA_CPUGPU static N EvaluateToSurfaceVector(const SDF& sdf, const N& position, float& outDistance)
	{
		// Base technique discussed at http://iquilezles.org/www/articles/normalsSDF/normalsSDF.htm

		if constexpr (std::is_same<N, glm::vec3>::value)
		{
			constexpr float H	= 0.001f;
			const glm::vec3 a	= {+1, -1, -1};
			const glm::vec3 b	= {-1, -1, +1};
			const glm::vec3 c	= {-1, +1, -1};
			const glm::vec3 d	= {+1, +1, +1};
			
			outDistance	= sdf.EvaluateDistance(position);

			return -glm::normalize(	a * (sdf.EvaluateDistance(position + a * H) - outDistance) + 
									b * (sdf.EvaluateDistance(position + b * H) - outDistance) + 
									c * (sdf.EvaluateDistance(position + c * H) - outDistance) + 
									d * (sdf.EvaluateDistance(position + d * H) - outDistance));
		}
		else if constexpr (std::is_same<N, glm::vec4>::value)
		{
			constexpr float H	= 0.005f;
			
			const glm::vec4 a = glm::vec4(0.250000f, 0.322749f, 0.456435f, -0.790569f);
			const glm::vec4 b = glm::vec4(0.250000f, 0.322749f, 0.456435f, 0.790569f);
			const glm::vec4 c = glm::vec4(0.250000f, 0.322749f, -0.912871f, 0.000000f);
			const glm::vec4 d = glm::vec4(0.264135f, -0.964486f, 0.000000f, 0.000000f);
			const glm::vec4 e = glm::vec4(-1.000000f, 0.000000f, 0.000000f, 0.000000f);
			
			outDistance	= sdf.EvaluateDistance(position);

			return -glm::normalize(	a * (sdf.EvaluateDistance(position + a * H) - outDistance) + 
									b * (sdf.EvaluateDistance(position + b * H) - outDistance) + 
									c * (sdf.EvaluateDistance(position + c * H) - outDistance) + 
									d * (sdf.EvaluateDistance(position + d * H) - outDistance) + 
									e * (sdf.EvaluateDistance(position + e * H) - outDistance));
		}
		
		// (Currently) Unsupported Vectortype
		// Can not throw an error in device code.
		return N();
	}
}