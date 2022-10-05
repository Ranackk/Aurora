#pragma once

#include <memory>

#include <cuda_runtime_api.h>

// We may not include mathlib here to avoid circular dependency!
#include "MathLib/VectorTypes.h"
#include "MathLib/Functions/Core.h"

#include "Rendering/CUDATypes.h"

template <typename N>
struct SDFEvaluateResult
{
public:

	using VectorType = N;

	float		SignedDistance = 0.0f;

	// Only set when sampling an inside of the SDF
	VectorType	Position;
	VectorType	Normal;
	glm::vec2	UV;

	void Invert();

	SDFEvaluateResult() = delete;
	A_CUDA_CPUGPU explicit SDFEvaluateResult(const float distance) : SignedDistance(distance), Position(), Normal(), UV() {}
	A_CUDA_CPUGPU explicit SDFEvaluateResult(const float distance, const VectorType& position, const VectorType& normal, const glm::vec2& uv) : SignedDistance(distance), Position(position), Normal(normal), UV(uv) {}

};

template <typename N>
void SDFEvaluateResult<N>::Invert()
{
	SignedDistance	*= -1.0f;
	Normal			*= -1.0f;
}