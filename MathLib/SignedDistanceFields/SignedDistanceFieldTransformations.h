#pragma once

#include <memory>
#include <type_traits>

#include <cuda_runtime_api.h>

#include "MathLib/SignedDistanceFields/SignedDistanceField.h"
#include "MathLib/SignedDistanceFields/SignedDistanceFieldTypes.h"
#include "MathLib/MathLib.h"
#include "MathLib/Functions/Core.h"

#include "Rendering/CUDATypes.h"

namespace Math
{
	//////////////////////////////////////////////////////////////////////////
	// SDF Modifiers - Transformations
	// partly taken from https://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
	//////////////////////////////////////////////////////////////////////////

	template <class T>
	class SDFTranslation : public SignedDistanceField
	{
	public:
		using VectorType = typename T::VectorType;
		using ResultType = SDFEvaluateResult<VectorType>;
		
		SDFTranslation() = delete;
		explicit SDFTranslation(T&& sdf, const VectorType& translation) :
			m_SDF(std::move(sdf)), m_Translation(translation) {}
			
		A_CUDA_CPUGPU T& GetSDF() { return m_SDF; }

		A_CUDA_CPUGPU inline void SetTranslationVector(const VectorType& translation) 
		{
			m_Translation = translation;	
		}

		//////////////////////////////////////////////////////////////////////////

		A_CUDA_CPUGPU inline float EvaluateDistance(const VectorType& position) const
		{
			return m_SDF.EvaluateDistance(position - m_Translation); 
		}

		//////////////////////////////////////////////////////////////////////////
		
		A_CUDA_CPUGPU inline VectorType GetLocalSamplePosition(const VectorType& position) const
		{
			return m_SDF.GetLocalSamplePosition(position - m_Translation);
		}

		//////////////////////////////////////////////////////////////////////////
		
		A_CUDA_CPUGPU inline VectorType GetTranslation() const
		{
			return m_Translation;
		}

	private:
		T			m_SDF;
		VectorType	m_Translation;
	};

	//////////////////////////////////////////////////////////////////////////

	template <class T>
	class SDFTransformation4x4 : public SignedDistanceField
	{
	public:
		using VectorType = typename T::VectorType;
		using ResultType = SDFEvaluateResult<VectorType>;

		SDFTransformation4x4() = delete;
		A_CUDA_CPUGPU explicit SDFTransformation4x4(T&& sdf, const glm::mat4x4& transformation) :
			m_SDF(std::move(sdf)), m_Transformation(transformation) {}
			
		A_CUDA_CPUGPU T& GetSDF() { return m_SDF; }

		//////////////////////////////////////////////////////////////////////////

		A_CUDA_CPUGPU void SetTransformationMatrix(const glm::mat4& matrix)
		{
			m_Transformation = matrix;
		}

		//////////////////////////////////////////////////////////////////////////

		A_CUDA_CPUGPU inline float EvaluateDistance(const VectorType& position) const
		{
			if constexpr (std::is_same<typename T::VectorType, glm::vec3>::value)
			{
				return m_SDF.EvaluateDistance(m_Transformation * glm::vec4(position, 1.0f));
			}
			else if constexpr (std::is_same<typename T::VectorType, glm::vec4>::value)
			{
				return m_SDF.EvaluateDistance(m_Transformation * position);
			}

			printf("Err: Not supported for this VectorType");
			return 1234;
		}

		//////////////////////////////////////////////////////////////////////////
		
		A_CUDA_CPUGPU inline VectorType GetLocalSamplePosition(const VectorType& position) const
		{
			return m_SDF.GetLocalSamplePosition(m_Transformation * position);
		}
		
	private:
		T			m_SDF;
		glm::mat4x4	m_Transformation;
	};

	//////////////////////////////////////////////////////////////////////////

}