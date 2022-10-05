#pragma once

#include <glm/glm.hpp>
#include "Marching/MarchingTypes.h"
#include "Rendering/CUDATypes.h"

namespace Math
{
	template <typename N>
	A_CUDA_CPUGPU ResultColor GetCheckerboard(const N& position, const float checkerSize = 10.0f)
	{
		unsigned int count = 0;
		for (int i = 0; i < static_cast<int>(N::length()); i++)
		{
			float wholeF;
			const float fraction = std::modf(position[i] / checkerSize + std::ceil(position[i] / checkerSize), &wholeF);

			count += static_cast<unsigned int>(std::floor(wholeF));
		}
		
		constexpr unsigned char colorOne = 50;
		constexpr unsigned char colorTwo = 200;

		const unsigned char color = count % 2 == 0 ? colorOne : colorTwo;
		return ResultColor{color, color, color, 255};
	}
}