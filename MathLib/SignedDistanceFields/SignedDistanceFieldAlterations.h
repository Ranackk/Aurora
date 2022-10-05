#pragma once

#include <cuda_runtime_api.h>

#include <memory>

#include "MathLib/SignedDistanceFields/SignedDistanceField.h"
#include "MathLib/SignedDistanceFields/SignedDistanceFieldTypes.h"
#include "MathLib/MathLib.h"
#include "MathLib/Functions/Core.h"

#include "Rendering/CUDATypes.h"

namespace Math
{
	//////////////////////////////////////////////////////////////////////////
	// SDF Modifiers - Booleans
	// partly taken from https://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
	//////////////////////////////////////////////////////////////////////////
	
	template <typename T>
	class SDFOnion : public SignedDistanceField
	{
	public:
		using VectorType = typename T::VectorType;
		using ResultType = SDFEvaluateResult<typename VectorType>;

		SDFOnion() = delete;
		A_CUDA_CPUGPU explicit SDFOnion(T&& sdf, const float thickness) :
			m_SDF(std::move(sdf)), m_Thickness(thickness) {}

		//////////////////////////////////////////////////////////////////////////

		A_CUDA_CPUGPU inline float EvaluateDistance(const VectorType& position) const
		{
			float distance			= m_SDF.EvaluateDistance(position);
			return std::abs(distance) - m_Thickness;
		}

	private:
		T		m_SDF;
		float	m_Thickness;

	};
	
	//////////////////////////////////////////////////////////////////////////
	
	template <typename T>
	class SDFRepetition : public SignedDistanceField
	{
	public:
		using VectorType = typename T::VectorType;
		using ResultType = SDFEvaluateResult<VectorType>;
		
		SDFRepetition() = delete;
		A_CUDA_CPUGPU explicit SDFRepetition(T&& sdf, const VectorType& spacing) :
			m_SDF(std::move(sdf)), m_Spacing(spacing) {}

		//////////////////////////////////////////////////////////////////////////

		A_CUDA_CPUGPU inline float EvaluateDistance(const VectorType& position) const
		{
			return m_SDF.EvaluateDistance(position + (0.5f * m_Spacing) % m_Spacing - 0.5f * m_Spacing); 
		}

	private:
		T			m_SDF;
		VectorType	m_Spacing;

	};
	
	//////////////////////////////////////////////////////////////////////////
	
	template <typename T>
	class SDFFiniteRepetition : public SignedDistanceField
	{
	public:
		using VectorType = typename T::VectorType;
		using ResultType = SDFEvaluateResult<VectorType>;
		
		SDFFiniteRepetition() = delete;
		A_CUDA_CPUGPU explicit SDFFiniteRepetition(T&& sdf, const VectorType& spacing, const VectorType& span) :
			m_SDF(std::move(sdf)), m_Spacing(spacing), m_Span(span) {}

		//////////////////////////////////////////////////////////////////////////

		A_CUDA_CPUGPU inline float EvaluateDistance(const VectorType& position) const
		{
			return m_SDF.EvaluateDistance(position - m_Spacing * glm::clamp(glm::round(position / m_Spacing), -m_Span, m_Span)); 
		}

	private:
		T			m_SDF;
		VectorType	m_Spacing;
		VectorType	m_Span;

	};
}