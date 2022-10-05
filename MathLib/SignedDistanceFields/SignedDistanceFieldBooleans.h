#pragma once

#include <memory>

#include <cuda_runtime_api.h>

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
	// Also look at https://www.iquilezles.org/www/articles/smin/smin.htm
	// for a further discussion on alternative implementations, including one that was done for "Dreams"
	//////////////////////////////////////////////////////////////////////////

	template <class T, class U>
	class SDFUnion : public SignedDistanceField
	{
		static_assert(std::is_same_v<T::VectorType, U::VectorType>, "SDF VectorType Missmatch");

	public:
		using VectorType = typename T::VectorType;

		SDFUnion() = delete;
		SDFUnion(T&& lhs, U&& rhs) :
			m_LHS(std::move(lhs)), m_RHS(std::move(rhs)) {}

		//////////////////////////////////////////////////////////////////////////

		A_CUDA_CPUGPU inline float EvaluateDistance(const VectorType& position) const
		{
			float resultLHS = m_LHS.EvaluateDistance(position);
			float resultRHS = m_RHS.EvaluateDistance(position);

			if (resultLHS <= resultRHS)
			{
				return resultLHS;
			}
			
			return resultRHS;
		}

		//////////////////////////////////////////////////////////////////////////
		
		A_CUDA_CPUGPU inline VectorType GetLocalSamplePosition(const VectorType& position) const
		{
			float resultLHS = m_LHS.EvaluateDistance(position);
			float resultRHS = m_RHS.EvaluateDistance(position);

			if (resultLHS <= resultRHS)
			{
				return m_LHS.GetLocalSamplePosition(position);
			}
			
			return m_RHS.GetLocalSamplePosition(position);
		}

		//////////////////////////////////////////////////////////////////////////
		
		A_CUDA_CPUGPU T* GetLHS() 
		{
			return &m_LHS; 
		}

		A_CUDA_CPUGPU U* GetRHS() 
		{
			return &m_RHS; 
		}

	private:
		T m_LHS;
		U m_RHS;
	};

	//////////////////////////////////////////////////////////////////////////

	template <class T, class U>
	class SDFSmoothUnion : public SignedDistanceField
	{
		static_assert(std::is_same_v<T::VectorType, U::VectorType>, "SDF VectorType Missmatch");

	public:
		using VectorType = typename T::VectorType;

		SDFSmoothUnion() = delete;
		explicit SDFSmoothUnion(T&& lhs, U&& rhs, const float smoothness) :
			m_LHS(std::move(lhs)), m_RHS(std::move(rhs)), m_Smoothness(smoothness) {}
			
		//////////////////////////////////////////////////////////////////////////

		A_CUDA_CPUGPU inline float EvaluateDistance(const VectorType& position) const
		{
			float distanceLHS	= m_LHS.EvaluateDistance(position);
			float distanceRHS	= m_RHS.EvaluateDistance(position);

			float h				= Math::Clamp01(0.5f + 0.5f * (distanceRHS - distanceLHS) / m_Smoothness);
			return Math::UnclampedLerp(distanceRHS, distanceLHS, h) - m_Smoothness * h * (1.0f - h);
		}

		//////////////////////////////////////////////////////////////////////////
		
		A_CUDA_CPUGPU inline VectorType GetLocalSamplePosition(const VectorType& position) const
		{
			const VectorType localPositionLHS = m_LHS.GetLocalSamplePosition(position);
			const VectorType localPositionRHS = m_RHS.GetLocalSamplePosition(position);

			const float distanceLHS	= m_LHS.EvaluateDistance(position);
			const float distanceRHS	= m_RHS.EvaluateDistance(position);

			const float h = Math::Clamp01(0.5f + 0.5f * (distanceRHS - distanceLHS) / m_Smoothness);
			return Math::UnclampedLerp(localPositionLHS, localPositionRHS, h) - m_Smoothness * h * (1.0f - h);
		}

		//////////////////////////////////////////////////////////////////////////
		
		A_CUDA_CPUGPU T* GetLHS() 
		{
			return &m_LHS; 
		}

		A_CUDA_CPUGPU U* GetRHS() 
		{
			return &m_RHS; 
		}

	private:
		T		m_LHS;
		U		m_RHS;
		float	m_Smoothness;

	};

	//////////////////////////////////////////////////////////////////////////

	template <class T, class U>
	class SDFIntersection : public SignedDistanceField
	{
		static_assert(std::is_same_v<T::VectorType, U::VectorType>, "SDF VectorType Missmatch");

	public:
		using VectorType = typename T::VectorType;
		using ResultType = SDFEvaluateResult<typename T::VectorType>;

		SDFIntersection() = delete;
		explicit SDFIntersection(T&& lhs, U&& rhs) :
			m_LHS(std::move(lhs)), m_RHS(std::move(rhs)) {}

		//////////////////////////////////////////////////////////////////////////

		A_CUDA_CPUGPU inline float EvaluateDistance(const VectorType& position) const
		{
			const float resultLHS = m_LHS.EvaluateDistance(position);
			const float resultRHS = m_RHS.EvaluateDistance(position);

			if (resultLHS >= resultRHS)
			{
				return resultLHS;
			}
				
			return resultRHS;
		}

		//////////////////////////////////////////////////////////////////////////
		
		A_CUDA_CPUGPU inline VectorType GetLocalSamplePosition(const VectorType& position) const
		{
			VectorType localPositionLHS, localPositionRHS;

			const float resultLHS = m_LHS.EvaluateDistance(position, localPositionLHS);
			const float resultRHS = m_RHS.EvaluateDistance(position, localPositionRHS);

			if (resultLHS >= resultRHS)
			{
				return m_LHS.GetLocalSamplePosition(position);
			}
			
			return m_RHS.GetLocalSamplePosition(position);
		}

		//////////////////////////////////////////////////////////////////////////
		
		A_CUDA_CPUGPU T* GetLHS() 
		{
			return &m_LHS; 
		}

		A_CUDA_CPUGPU U* GetRHS() 
		{
			return &m_RHS; 
		}
		
	private:
		T m_LHS;
		U m_RHS;
	};

	//////////////////////////////////////////////////////////////////////////

	template <class T, class U>
	class SDFSmoothIntersection : public SignedDistanceField
	{
		static_assert(std::is_same_v<T::VectorType, U::VectorType>, "SDF VectorType Missmatch");

	public:
		using VectorType = typename T::VectorType;
		
		SDFSmoothIntersection() = delete;
		explicit SDFSmoothIntersection(T&& lhs, U&& rhs, const float smoothness) :
			m_LHS(std::move(lhs)), m_RHS(std::move(rhs)), m_Smoothness(smoothness) {}

		//////////////////////////////////////////////////////////////////////////
		
		A_CUDA_CPUGPU inline float EvaluateDistance(const VectorType& position) const
		{
			const float distanceLHS	= m_LHS.EvaluateDistance(position);
			const float distanceRHS	= m_RHS.EvaluateDistance(position);

			const float h = Math::Clamp01(0.5f - 0.5f * (distanceRHS - distanceLHS) / m_Smoothness);
			return Math::UnclampedLerp(distanceRHS, distanceLHS, h) + m_Smoothness * h * (1.0f - h);
		}

		//////////////////////////////////////////////////////////////////////////
		
		A_CUDA_CPUGPU inline VectorType GetLocalSamplePosition(const VectorType& position) const
		{
			const VectorType localPositionLHS = m_LHS.GetLocalSamplePosition(position);
			const VectorType localPositionRHS = m_RHS.GetLocalSamplePosition(position);

			const float distanceLHS	= m_LHS.EvaluateDistance(position);
			const float distanceRHS	= m_RHS.EvaluateDistance(position);
			
			const float h = Math::Clamp01(0.5f - 0.5f * (distanceRHS - distanceLHS) / m_Smoothness);
			return Math::UnclampedLerp(localPositionLHS, localPositionRHS, h) + m_Smoothness * h * (1.0f - h);
		}

		//////////////////////////////////////////////////////////////////////////
		
		A_CUDA_CPUGPU T* GetLHS() 
		{
			return &m_LHS; 
		}

		A_CUDA_CPUGPU U* GetRHS() 
		{
			return &m_RHS; 
		}
		
	private:
		T		m_LHS;
		U		m_RHS;
		float	m_Smoothness;
	};

	//////////////////////////////////////////////////////////////////////////

	// Substracts lhs form rhs
	template <class T, class U>
	class SDFSubstraction : public SignedDistanceField
	{
		static_assert(std::is_same_v<T::VectorType, U::VectorType>, "SDF VectorType Missmatch");

	public:

		using VectorType = typename T::VectorType;
		using ResultType = SDFEvaluateResult<typename T::VectorType>;

		SDFSubstraction() = delete;
		explicit SDFSubstraction(T&& lhs, U&& rhs) :
			m_LHS(std::move(lhs)), m_RHS(std::move(rhs)) {}
			
		//////////////////////////////////////////////////////////////////////////

		A_CUDA_CPUGPU inline float EvaluateDistance(const VectorType& position) const
		{
			float resultLHS = m_LHS.EvaluateDistance(position);
			const float resultRHS = m_RHS.EvaluateDistance(position);
			resultLHS *= -1;

			if (resultLHS >= resultRHS)
			{
				return resultLHS;
			}

			return resultRHS;
		}

		//////////////////////////////////////////////////////////////////////////
		
		A_CUDA_CPUGPU inline VectorType GetLocalSamplePosition(const VectorType& position) const
		{
			VectorType localPositionLHS, localPositionRHS;

			const float resultLHS = m_LHS.EvaluateDistance(position);
			const float resultRHS = m_RHS.EvaluateDistance(position);
			resultLHS *= -1;

			if (resultLHS >= resultRHS)
			{
				return m_LHS.GetLocalSamplePosition(position);
			}
			
			return m_RHS.GetLocalSamplePosition(position);
		}

		//////////////////////////////////////////////////////////////////////////
		
		A_CUDA_CPUGPU T* GetLHS() 
		{
			return &m_LHS; 
		}

		A_CUDA_CPUGPU U* GetRHS() 
		{
			return &m_RHS; 
		}

	private:
		T m_LHS;
		U m_RHS;
	};

	//////////////////////////////////////////////////////////////////////////

	template <class T, class U>
	class SDFSmoothSubstraction : public SignedDistanceField
	{
		static_assert(std::is_same_v<T::VectorType, U::VectorType>, "SDF VectorType Missmatch");

	public:
		using VectorType = typename T::VectorType;

		SDFSmoothSubstraction() = delete;
		explicit SDFSmoothSubstraction(T&& lhs, U&& rhs, const float smoothness) :
			m_LHS(std::move(lhs)), m_RHS(std::move(rhs)), m_Smoothness(smoothness) {}

		//////////////////////////////////////////////////////////////////////////

		A_CUDA_CPUGPU inline float EvaluateDistance(const VectorType& position) const
		{
			const float distanceRHS = m_LHS.EvaluateDistance(position);
			const float distanceLHS	= m_RHS.EvaluateDistance(position);

			const float h = Math::Clamp01(0.5f - 0.5f * (distanceRHS + distanceLHS) / m_Smoothness);
			return Math::UnclampedLerp(distanceRHS, -distanceLHS, h) + m_Smoothness * h * (1.0f - h);
		}

		//////////////////////////////////////////////////////////////////////////
		
		A_CUDA_CPUGPU inline VectorType GetLocalSamplePosition(const VectorType& position) const
		{
			const VectorType localPositionLHS = m_LHS.GetLocalSamplePosition(position);
			const VectorType localPositionRHS = m_RHS.GetLocalSamplePosition(position);

			const float distanceLHS	= m_LHS.EvaluateDistance(position);
			const float distanceRHS	= m_RHS.EvaluateDistance(position);
			
			const float h = Math::Clamp01(0.5f - 0.5f * (distanceRHS + distanceLHS) / m_Smoothness);
			return Math::UnclampedLerp(localPositionLHS, -localPositionRHS, h) + m_Smoothness * h * (1.0f - h);
		}	

		//////////////////////////////////////////////////////////////////////////
		
		A_CUDA_CPUGPU T* GetLHS() 
		{
			return &m_LHS; 
		}

		A_CUDA_CPUGPU U* GetRHS() 
		{
			return &m_RHS; 
		}
	
	private:
		T		m_LHS;
		U		m_RHS;
		float	m_Smoothness;
	};
}

