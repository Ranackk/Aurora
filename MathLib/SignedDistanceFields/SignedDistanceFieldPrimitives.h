#pragma once

#include <memory>

#include <cuda_runtime_api.h>

#include "MathLib/SignedDistanceFields/SignedDistanceField.h"
#include "MathLib/SignedDistanceFields/SignedDistanceFieldTypes.h"
#include "MathLib/MathLib.h"
#include "MathLib/Functions/Core.h"

//#include "Raymarching/RayMarchFunctions.h"

#include "Rendering/CUDATypes.h"

struct Material;

//////////////////////////////////////////////////////////////////////////

namespace Math
{
	//////////////////////////////////////////////////////////////////////////
	// SDF Primitives
	// partly taken from https://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// 2-Dimensional
	//////////////////////////////////////////////////////////////////////////

	// glm::vec2 does not support per component multiplication, which makes some problems for normal calculation.
	// As a result, 2D stuff is disabled.

	//class SDFCircle : public SignedDistanceField<glm::vec2>
	//{
	//private:
	//	using ResultType = SDFEvaluateResult<glm::vec2>;

	//	std::shared_ptr<const Material>	m_Material;
	//	float							m_Radius;

	//	//////////////////////////////////////////////////////////////////////////

	//public:

	//	SDFCircle() = delete;
	//	SDFCircle(std::shared_ptr<const Material> material, float radius) : m_Material(material), m_Radius(radius) {}

	//	//////////////////////////////////////////////////////////////////////////

	//	//virtual inline ResultType Evaluate(const glm::vec2& position) const override
	//	//{
	//	//	float distance = Math::Length(position) - m_Radius;
	//	//	if (distance > OptionsManager::RAY_HIT_EPSILON)
	//	//	{
	//	//		ResultType result = ResultType(distance);
	//	//		return result;
	//	//	}


	//	//	glm::vec2 normal = glm::normalize(position);
	//	//	ResultType result = ResultType(distance, position, normal, glm::vec2(), m_Material);
	//	//	return result;
	//	//}

	//	//////////////////////////////////////////////////////////////////////////

	//	virtual inline float EvaluateDistance(const glm::vec2& position) const override
	//	{
	//		float distance = Math::Length(position) - m_Radius;
	//		return distance;
	//	}

	//};

	//////////////////////////////////////////////////////////////////////////
	// 3-Dimensional
	//////////////////////////////////////////////////////////////////////////

	class SDFSphere
	{
	public:
		using VectorType = glm::vec3;
		using ResultType = SDFEvaluateResult<VectorType>;

		SDFSphere() = delete;
		A_CUDA_CPUGPU explicit SDFSphere(const float radius) : m_Radius(radius) {}

		//////////////////////////////////////////////////////////////////////////

		A_CUDA_CPUGPU virtual inline float EvaluateDistance(const VectorType& position) const
		{
			const float distance = Math::Length(position) - m_Radius;
			return distance;
		}

		//////////////////////////////////////////////////////////////////////////
		
		A_CUDA_CPUGPU inline VectorType GetLocalSamplePosition(const VectorType& position) const
		{
			return position;
		}

	private:
		float m_Radius;

	};

	//////////////////////////////////////////////////////////////////////////
	// 4-Dimensional
	//////////////////////////////////////////////////////////////////////////
	
	template <class N>
	class SDFBox
	{
	public:
		using VectorType = N;
		using ResultType = SDFEvaluateResult<VectorType>;

		SDFBox() = delete;
		A_CUDA_CPUGPU explicit SDFBox(const VectorType& extents) : m_Extents(extents) {}

		//////////////////////////////////////////////////////////////////////////

		A_CUDA_CPUGPU inline float EvaluateDistance(const VectorType& position) const
		{
			const glm::vec4 q		= Math::Abs(position) - m_Extents;
			const float distance	= Math::Min(Math::MaxComponent(q), 0.0f) + Math::Length(Math::Max(q, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f)));
			return distance;
		}

		//////////////////////////////////////////////////////////////////////////
		
		A_CUDA_CPUGPU inline VectorType GetLocalSamplePosition(const VectorType& position) const
		{
			return position;
		}

	private:
		VectorType m_Extents;
	};

	//////////////////////////////////////////////////////////////////////////
	// 4-Dimensional
	//////////////////////////////////////////////////////////////////////////

	class SDFHyperSphere : public SignedDistanceField
	{
	public:
		using VectorType = glm::vec4;
		using ResultType = SDFEvaluateResult<VectorType>;

		SDFHyperSphere() = delete;
		A_CUDA_CPUGPU explicit SDFHyperSphere(const float radius) : m_Radius(radius) {}

		//////////////////////////////////////////////////////////////////////////

		A_CUDA_CPUGPU inline float EvaluateDistance(const VectorType& position) const
		{
			const float distance = Math::Length(position) - m_Radius;
			return distance;
		}

		//////////////////////////////////////////////////////////////////////////
		
		A_CUDA_CPUGPU inline VectorType GetLocalSamplePosition(const VectorType& position) const
		{
			return position;
		}

	private:
		float m_Radius;
	};

	//////////////////////////////////////////////////////////////////////////

}