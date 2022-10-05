#pragma once

#include "Rendering/Scenes/Scene.h"

#include "MathLib/MathLib.h"
#include "MathLib/SignedDistanceFields/SignedDistanceField.h"

struct Configuration;

class A_CPUGPU_ALIGN(64) SceneHyperPlayground : public Scene<glm::vec4>
{
public:
	using N = glm::vec4;

	void Init();
	void UnInit();
	void Update(Configuration& config);

	A_CUDA_CPUGPU float EvaluateDistance(const glm::vec4& position) const;
	A_CUDA_CPUGPU glm::vec4 EvaluateNormal(const glm::vec4& position) const;
	A_CUDA_CPUGPU glm::vec4 EvaluateToSurfaceVector(const glm::vec4& position, float& outDistance) const;
	A_CUDA_CPUGPU glm::vec4 EvaluateToSurfaceVectorZW(const glm::vec4& position, float& outDistance) const;
	A_CUDA_CPUGPU glm::vec4 GetLocalSamplePosition(const glm::vec4& position) const;

private:
	Math::SDFTranslation<Math::SDFUnion<Math::SDFTranslation<Math::SDFTransformation4x4<Math::SDFBox<glm::vec4>>>,Math::SDFTranslation<Math::SDFBox<glm::vec4>>>>* m_SDF = nullptr;
	Math::SDFTranslation<Math::SDFBox<glm::vec4>>* m_SDF_Plane = nullptr;
	Math::SDFTranslation<Math::SDFTransformation4x4<Math::SDFBox<glm::vec4>>>* m_SDF_Cube = nullptr;

	glm::vec4 m_BaseTranslation = glm::vec4(0, 0, 0, 0);
};

// Use this to find the decltype result (inside a function definition): typename decltype(result)::_;
// Use this to find out class sizes.
// template<size_t S> class Sizer { }; Sizer<sizeof(Math::SDFUnion<Math::SDFBox<glm::vec4>, Math::SDFHyperSphere>)> foo;
