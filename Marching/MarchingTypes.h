#pragma once

#include "GraphicsIncludes.h"
#include <Vendor/imgui/imgui.h>

#include <functional>
#include "Rendering/CUDATypes.h"

//////////////////////////////////////////////////////////////////////////

using BufferType = unsigned char;

struct ResultColor
{
	BufferType Red = 0;
	BufferType Green = 0;
	BufferType Blue = 0;
	BufferType Alpha = 0;
	
	A_CUDA_CPUGPU operator uchar4() const
	{
		return {Red, Green, Blue, Alpha};
	}

	//////////////////////////////////////////////////////////////////////////

	operator ImVec4() const
	{
		return {Red / 255.0f, Green / 255.0f, Blue / 255.0f, Alpha / 255.0f};
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU ResultColor operator *(const float factor) const
	{
		return {static_cast<BufferType>(Red * factor), 
				static_cast<BufferType>(Green * factor), 
				static_cast<BufferType>(Blue * factor), 
				Alpha};
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU ResultColor operator +(const ResultColor& otherColor) const
	{
		return {static_cast<BufferType>(Red + otherColor.Red), 
				static_cast<BufferType>(Green + otherColor.Green), 
				static_cast<BufferType>(Blue + otherColor.Blue), 
				Alpha};
	}
};

//////////////////////////////////////////////////////////////////////////

template <class N>
struct RayMarchResult
{
	bool				Hit = false;
	unsigned int		Steps = 0;
	
	float				SignedDistance = 0.0f;
	N					LocalPosition = N();
	N					Position = N();
	N					ClosestPosition = N();
	N					Normal = N();
	N					LocalNormal = N();

	float				ShadowValue = 0.0f;
	
	float				TraversedPrimary = 0.0f;
	float				TraversedSecondary = 0.0f;

	A_CUDA_CPUGPU RayMarchResult() = default;
	A_CUDA_CPUGPU explicit RayMarchResult(const bool hit, const float distance, const unsigned int steps, const float traversedPrimary, const float traversedSecondary, const N& position, const N& localPosition, const N& closestPosition)										: Hit(hit), Steps(steps), SignedDistance(distance), TraversedPrimary(traversedPrimary), TraversedSecondary(traversedSecondary), Position(position), LocalPosition(localPosition), ClosestPosition(closestPosition), Normal(N()), LocalNormal(N()) {}
	A_CUDA_CPUGPU explicit RayMarchResult(const bool hit, const float distance, const unsigned int steps, const float traversedPrimary, const float traversedSecondary, const N& position, const N& localPosition, const N& closestPosition, const N& normal, const N& localNormal)	: Hit(hit), Steps(steps), SignedDistance(distance), TraversedPrimary(traversedPrimary), TraversedSecondary(traversedSecondary), Position(position), LocalPosition(localPosition), ClosestPosition(closestPosition), Normal(normal), LocalNormal(localNormal) {}
};

//////////////////////////////////////////////////////////////////////////
	
struct RenderingBuffer
{
	GLuint					TextureHandle			= 0;
	cudaGraphicsResource_t	d_CUDAGraphicsResource	= nullptr;
	bool					IsCurrentlyMapped		= false;
};

//////////////////////////////////////////////////////////////////////////