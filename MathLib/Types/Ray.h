#pragma once

#include <glm\ext\vector_float2.hpp>
#include "Rendering/CUDATypes.h"

namespace Math
{
	template <class N>
	struct Ray
	{
		N Origin;
		N Direction;

		//////////////////////////////////////////////////////////////////////////

		A_CUDA_CPUGPU Ray()												: Origin(), Direction() {}
		A_CUDA_CPUGPU explicit Ray(const N& origin, const N& direction)	: Origin(origin), Direction(direction) {}

		//////////////////////////////////////////////////////////////////////////

		A_CUDA_CPUGPU inline void Set(const N& origin, const N& direction)
		{
			Origin		= origin;
			Direction	= direction;
		}

		//////////////////////////////////////////////////////////////////////////

		A_CUDA_CPUGPU inline N At(const float traverseDistance) const
		{
			return Origin + Direction * traverseDistance;
		}
	};

	//////////////////////////////////////////////////////////////////////////

	template <class N>
	struct BiRay
	{
		N Origin;
		N DirectionMain;
		N DirectionSecondary;

		A_CUDA_CPUGPU BiRay()																				: Origin(), DirectionMain(), DirectionSecondary() {}
		A_CUDA_CPUGPU explicit BiRay(const N& origin, const N& directionMain, const N& directionSecondary)	: Origin(origin), DirectionMain(directionMain), DirectionSecondary(directionSecondary) {}

		//////////////////////////////////////////////////////////////////////////

		A_CUDA_CPUGPU inline N At(const float traverseDistanceMain, const float traverseDistanceSecondary) const
		{
			return Origin + DirectionMain * traverseDistanceMain + DirectionSecondary * traverseDistanceSecondary;
		}
	};
}