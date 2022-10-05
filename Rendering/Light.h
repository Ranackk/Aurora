#pragma once

#include "Rendering/CUDATypes.h"

template <typename VectorType>
struct A_CPUGPU_ALIGN(32) Light 
{
	Light() = default;

	VectorType	Position;
	float		Radius;

	//////////////////////////////////////////////////////////////////////////

	void Initialize(const VectorType& position, const float radius)
	{
		Position	= position;
		Radius		= radius;
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU VectorType GetPosition()
	{
		return Position;
	}
};


