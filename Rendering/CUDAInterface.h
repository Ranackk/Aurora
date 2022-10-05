#pragma once

#include "MathLib/MathLib.h"
#include "Marching/MarchingTypes.h"

#include "Rendering/CUDATypes.h"
#include "Rendering/Scenes/SceneHyperPlayground.h"

class SceneHyperPlayground;

struct RenderPixelBufferDataCUDA
{		
	// Buffer Data
	cudaSurfaceObject_t SurfaceObject		= 0;
	glm::ivec2	BufferDimensions			= {0, 0};

	glm::ivec2	ViewDimensions				= {0, 0};
	glm::ivec2	NumViews					= {0, 0};

	RenderPixelBufferDataCUDA() = default;
	A_CUDA_CPUGPU void Initialize(const cudaSurfaceObject_t surfaceObject, const glm::ivec2& bufferDimensions, const glm::ivec2& viewDimensions, const glm::ivec2& numViews)
	{
		SurfaceObject		= surfaceObject;
		BufferDimensions	= bufferDimensions;
		ViewDimensions		= viewDimensions;
		NumViews			= numViews;
	}
};

//////////////////////////////////////////////////////////////////////////

struct RenderVoxelDataCUDA
{
	using ResultType = RayMarchResult<glm::vec4>;
	ResultType Result;

	RenderVoxelDataCUDA() = delete;
	explicit RenderVoxelDataCUDA(const ResultType& result) : Result(result) {};
};

//////////////////////////////////////////////////////////////////////////

struct RenderVoxelBufferDataCUDA
{
	// Buffer Data
	glm::ivec3 VolumeOriginWS	= {0, 0, 0};
	glm::ivec3 VolumeSizeWS		= {20, 20, 20};
	glm::ivec3 BufferDimensions	= {20, 20, 20};

	RenderVoxelDataCUDA* d_VolumeBuffer = nullptr;

	RenderVoxelBufferDataCUDA() = default;
	A_CUDA_CPUGPU void Initialize(RenderVoxelDataCUDA* const d_volumeBuffer, const glm::ivec3& bufferDimensions, const glm::ivec3& volumeOriginWS, const glm::ivec3& volumeSizeWS)
	{
		d_VolumeBuffer		= d_volumeBuffer;
		BufferDimensions	= bufferDimensions;
		VolumeOriginWS		= volumeOriginWS;
		VolumeSizeWS		= volumeSizeWS;
	}
};


////////////////////////////////////////////////////////////////

struct A_CPUGPU_ALIGN(8) RenderSceneDataCUDA
{
	// Supports only 4D data for now.

	// Scene
	// We wrap the scene in here to make it easier changeable later, as managed memory does not support virtual functions.
	// Needs to have a .Evaluate() function.
	SceneHyperPlayground* mcm_Scene;

	RenderSceneDataCUDA() = default;
	A_CUDA_CPUGPU void Initialize(SceneHyperPlayground* const scene)
	{
		mcm_Scene = scene;
	}
	
	////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU float EvaluateDistance(const glm::vec4& position) const
	{
		return mcm_Scene->EvaluateDistance(position);
	}

	////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU glm::vec4 EvaluateNormal(const glm::vec4& position) const
	{
		const glm::vec4 norm = mcm_Scene->EvaluateNormal(position);
		return norm;
	}
	////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU glm::vec4 EvaluateToSurfaceVector(const glm::vec4& position, float& outDistance) const
	{
		const glm::vec4 norm = mcm_Scene->EvaluateToSurfaceVector(position, outDistance);
		return norm;
	}
	
	////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU glm::vec4 EvaluateToSurfaceVectorZW(const glm::vec4& position, float& outDistance) const
	{
		const glm::vec4 norm = mcm_Scene->EvaluateToSurfaceVectorZW(position, outDistance);
		return norm;
	}

	////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU glm::vec4 GetLocalSamplePosition(const glm::vec4& position) const
	{
		return mcm_Scene->GetLocalSamplePosition(position);
	}
};
