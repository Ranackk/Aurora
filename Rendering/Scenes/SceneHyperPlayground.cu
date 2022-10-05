#include "stdafx.h"

#include "Rendering/Scenes/SceneHyperPlayground.h"
#include "Rendering/Scenes/SceneTypes.h"
#include "Rendering/Scenes/SDFFactory.h"

#include "Options/Configuration.h"

#include "MathLib/MathLib.h"
#include "../Application.h"

//////////////////////////////////////////////////////////////////////////

void SceneHyperPlayground::Update(Configuration& config) {	
	auto time = Application::s_Instance->GetTimeSinceStartupMyS();
	for (int i = 0; i < 6; i++)
	{
		if (config.SceneAnimateRotations[i]) 
		{
			config.SceneSliderRotations[i] = fmod((time.count() / 1000000.0f * config.SceneSpeed), glm::two_pi<float>());
		}
	}

	glm::mat4 transformation = Math::RotZW(config.SceneSliderRotations[0]) * 
							   Math::RotYW(config.SceneSliderRotations[1]) * 
							   Math::RotYZ(config.SceneSliderRotations[2]) * 
							   Math::RotXW(config.SceneSliderRotations[3]) * 
							   Math::RotXZ(config.SceneSliderRotations[4]) *
							   Math::RotXY(config.SceneSliderRotations[5]);

	glm::vec4 translation	= {config.SceneSliderPositions[0], config.SceneSliderPositions[1], config.SceneSliderPositions[2], config.SceneSliderPositions[3]};

	m_SDF_Cube->GetSDF().SetTransformationMatrix(transformation);
	m_SDF_Cube->SetTranslationVector(m_BaseTranslation + translation);
}

//////////////////////////////////////////////////////////////////////////

A_CUDA_CPUGPU float SceneHyperPlayground::EvaluateDistance(const glm::vec4& position) const
{
	return m_SDF_Cube->EvaluateDistance(position);
}

//////////////////////////////////////////////////////////////////////////

A_CUDA_CPUGPU glm::vec4 SceneHyperPlayground::EvaluateNormal(const glm::vec4& position) const
{
	return Math::SampleNormal(*m_SDF_Cube, position);
}

//////////////////////////////////////////////////////////////////////////

A_CUDA_CPUGPU glm::vec4 SceneHyperPlayground::GetLocalSamplePosition(const glm::vec4& position) const
{
	return m_SDF_Cube->GetLocalSamplePosition(position);
}

//////////////////////////////////////////////////////////////////////////

A_CUDA_CPUGPU glm::vec4 SceneHyperPlayground::EvaluateToSurfaceVectorZW(const glm::vec4& position, float& outDistance) const
{
	return Math::EvaluateToSurfaceVectorZW(*m_SDF_Cube, position, outDistance);
}

//////////////////////////////////////////////////////////////////////////

A_CUDA_CPUGPU glm::vec4 SceneHyperPlayground::EvaluateToSurfaceVector(const glm::vec4& position, float& outDistance) const
{
	return Math::EvaluateToSurfaceVector(*m_SDF_Cube, position, outDistance);
}

//////////////////////////////////////////////////////////////////////////

void SceneHyperPlayground::Init()
{
	m_SDF				= SDFFactory::CreateSDF_HyperCube();
	m_SDF_Plane			= m_SDF->GetSDF().GetRHS();
	m_SDF_Cube			= m_SDF->GetSDF().GetLHS();

	m_BaseTranslation	= m_SDF_Cube->GetTranslation();
}

//////////////////////////////////////////////////////////////////////////

void SceneHyperPlayground::UnInit()
{
	delete m_SDF;
}
