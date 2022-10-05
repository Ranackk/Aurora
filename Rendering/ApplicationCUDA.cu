 #pragma once

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cstdio>

#include "GraphicsIncludes.h"

#include "Marching/MarchingTypes.h"
#include "Marching/MarchingFunctions.h"
#include "MathLib/MathLib.h"

#include "Rendering/CUDATypes.h"
#include "Rendering/CUDAInterface.h"
#include "Rendering/Camera.h"
#include "Rendering/Light.h"

#include "Options/Configuration.h"


#ifdef __CUDACC__
#define KERNEL_ARGS2(grid, block) <<< grid, block >>>
#define KERNEL_ARGS3(grid, block, sh_mem) <<< grid, block, sh_mem >>>
#define KERNEL_ARGS4(grid, block, sh_mem, stream) <<< grid, block, sh_mem, stream >>>
#else
#define KERNEL_ARGS2(grid, block)
#define KERNEL_ARGS3(grid, block, sh_mem)
#define KERNEL_ARGS4(grid, block, sh_mem, stream)
#endif

constexpr unsigned int BLOCK_SIZE_2D = 16;
//constexpr unsigned int BLOCK_SIZE_3D = 8;
constexpr unsigned int RESULT_COLOR_COMPONENT_COUNT = 4;

////////////////////////////////////////////////////////////////

using N = glm::vec4;

A_CUDA_KERNEL void k_RenderPixel(RenderPixelBufferDataCUDA* bufferData, RenderSceneDataCUDA* sceneData, Configuration* config, Camera<glm::vec4>* camera, Light<glm::vec4>* light);

void CUDA_RenderImage(RenderPixelBufferDataCUDA* bufferData, RenderSceneDataCUDA* sceneData, Configuration* config, Camera<glm::vec4>* camera, Light<glm::vec4>* light);

void CUDA_PrepareRenderImage(RenderingBuffer& RenderingBuffer, cudaSurfaceObject_t& outSurfaceObject);
void CUDA_FinishRenderImage(RenderingBuffer& RenderingBuffer);

////////////////////////////////////////////////////////////////
// Slice Rendering (4D -> 2D)
////////////////////////////////////////////////////////////////

// Needs to be called form OpenGL Thread
void CUDA_PrepareRenderImage(RenderingBuffer& RenderingBuffer, cudaSurfaceObject_t& outSurfaceObject)
{
	if (!RenderingBuffer.IsCurrentlyMapped)
	{
		CUDA_CHECK_ERROR(cudaGraphicsMapResources(1, &RenderingBuffer.d_CUDAGraphicsResource));
		RenderingBuffer.IsCurrentlyMapped = true;
	}
	
	cudaArray_t arrayDPtr;
	CUDA_CHECK_ERROR(cudaGraphicsSubResourceGetMappedArray(&arrayDPtr, RenderingBuffer.d_CUDAGraphicsResource, 0, 0));

	// Create the cuda resource description
	struct cudaResourceDesc resDesc;
	memset(&resDesc, 0, sizeof(resDesc));
	resDesc.resType			= cudaResourceTypeArray;    // be sure to set the resource type to cudaResourceTypeArray
	resDesc.res.array.array = arrayDPtr;				// this is the important bit
 
	// Create the surface object
	CUDA_CHECK_ERROR(cudaCreateSurfaceObject(&outSurfaceObject, &resDesc));
}

// Needs to be called form OpenGL Thread
void CUDA_FinishRenderImage(RenderingBuffer& RenderingBuffer)
{
	if (RenderingBuffer.IsCurrentlyMapped)
	{
		CUDA_CHECK_ERROR(cudaGraphicsUnmapResources(1, &RenderingBuffer.d_CUDAGraphicsResource));
		RenderingBuffer.IsCurrentlyMapped = false;
	}
}

// May be called from any Thread
void CUDA_RenderImage(RenderPixelBufferDataCUDA* bufferData, RenderSceneDataCUDA* sceneData, Configuration* config, Camera<glm::vec4>* camera, Light<glm::vec4>* light)
{	
	CUDA_CHECK_ERROR(cudaGetLastError());
	
	////////////////////////////////////////////////////////////////

	//printf("Start Render Image\n");
	const dim3 threadsPerBlock	= dim3(BLOCK_SIZE_2D, BLOCK_SIZE_2D);
	const dim3 numBlocks		= dim3(bufferData->BufferDimensions.x / BLOCK_SIZE_2D, bufferData->BufferDimensions.y /BLOCK_SIZE_2D);

	constexpr bool SHOW_DEBUG = false;
	if (SHOW_DEBUG)
	{
		// 32 * 32 = 1024;
		struct cudaDeviceProp properties;
		cudaGetDeviceProperties(&properties, 0);
		printf("using %i multiprocessors\n", properties.multiProcessorCount);
		printf("max threads per processor: %i\n", properties.maxThreadsPerMultiProcessor);
		printf("params: threadsPerBlock (%i, %i), numBlocks (%i, %i) \n", threadsPerBlock.x, threadsPerBlock.y, numBlocks.x, numBlocks.y);
	}

	cudaDeviceSynchronize();
	CUDA_CHECK_ERROR(cudaGetLastError());

	k_RenderPixel KERNEL_ARGS2(numBlocks, threadsPerBlock)(bufferData, sceneData, config, camera, light);
	
	cudaDeviceSynchronize();
	CUDA_CHECK_ERROR(cudaGetLastError());
	
	////////////////////////////////////////////////////////////////

	//printf("End Render Image\n");
  }

////////////////////////////////////////////////////////////////

A_CUDA_KERNEL void k_RenderPixel(RenderPixelBufferDataCUDA* bufferData, RenderSceneDataCUDA* sceneData, Configuration* config, Camera<glm::vec4>* camera, Light<glm::vec4>* light)
{
	#define USE_BIRAY_MARCHING

	// Global

	const int pixelX		= blockIdx.x * blockDim.x + threadIdx.x;
	const int pixelY		= blockIdx.y * blockDim.y + threadIdx.y;
	
	const int viewX			= pixelX / bufferData->ViewDimensions.x; 
	const int viewY			= pixelY / bufferData->ViewDimensions.y;
	const int viewID		= viewY * bufferData->NumViews.x + viewX;
	const int viewCount		= bufferData->NumViews.x * bufferData->NumViews.y;

	const int viewOriginX	= viewX * bufferData->ViewDimensions.x;
	const int viewOriginY	= viewY * bufferData->ViewDimensions.y;

	const float viewPercentage	= (viewCount == 1) ? 0.5f : viewID / static_cast<float>(viewCount);

	// In View

	const int inViewX		= pixelX - viewOriginX;
	const int inViewY		= pixelY - viewOriginY;
	
	// Ray
	const float inViewPercentageX = inViewX / static_cast<float>(bufferData->ViewDimensions.x);
	const float inViewPercentageY = inViewY / static_cast<float>(bufferData->ViewDimensions.y);
	
	constexpr float SCISSOR_RECT_SIZE_X			= 0.40f;
	constexpr float SCISSOR_RECT_SIZE_X_HALF	= SCISSOR_RECT_SIZE_X / 2.0f;
	constexpr float SCISSOR_RECT_SIZE_Y			= 0.60f;
	constexpr float SCISSOR_RECT_SIZE_Y_HALF	= SCISSOR_RECT_SIZE_Y / 2.0f;
	constexpr float GROUND_PLANE_Y				= 0.0f;

	const bool isInGroundPlane = inViewPercentageY < GROUND_PLANE_Y;
	const bool isInScissorRect = inViewPercentageX > (0.5f - SCISSOR_RECT_SIZE_X_HALF) && inViewPercentageX < (0.5f + SCISSOR_RECT_SIZE_X_HALF) &&
		 				   inViewPercentageY > (0.5f - SCISSOR_RECT_SIZE_Y_HALF) && inViewPercentageY < (0.5f + SCISSOR_RECT_SIZE_Y_HALF);
	
	// March Ray

	RayMarchResult<glm::vec4> result;
	if (isInGroundPlane)
	{
		// Render Ground Plane via Raytracing

		// Calculate intersection point between ray and plane. Note: We do only use a ray here, not a biray.
		glm::highp_mat4 biRaySpaceToWorldSpace;
		const Math::BiRay<glm::vec4> biRay	= camera->GetBiray(viewPercentage, inViewPercentageX, inViewPercentageY, biRaySpaceToWorldSpace);

		constexpr float GROUND_POSITION_Y	= -100.0f;
		const float traversedMain			= GROUND_POSITION_Y - biRay.Origin.y / biRay.DirectionMain.y;
		const glm::vec4 position			= biRay.At(traversedMain, 0);
		const glm::vec4 normal				= glm::vec4(0, 1, 0, 0);

		result = RayMarchResult<glm::vec4>(true, traversedMain, 1, 0, 0, position, position, position, normal, normal);
	}
	else if (isInScissorRect)
	{
		// Render Scene	via WaveMarching	

		glm::highp_mat4 biRaySpaceToWorldSpace;
		const Math::BiRay<glm::vec4> biRay	= camera->GetBiray(viewPercentage, inViewPercentageX, inViewPercentageY, biRaySpaceToWorldSpace);
		result								= RayMarchFunctions::MarchSingleBiRay<glm::vec4, glm::mat4>(biRay, biRaySpaceToWorldSpace, sceneData, config->MIN_STEP_SIZE, config->MAX_DEPTH, config->MAX_STEPS, config->RAY_HIT_EPSILON);
	}
	else
	{
		// Neither the main scene nor the ground plane is rendered, so we do not alter the "not hit" result.
		result = RayMarchResult<glm::vec4>();
		result.Hit = false;
	}

	// Soft shadows
	if (result.Hit)
	{
		const glm::vec4 toLightPosition		= light->Position - result.Position;
		const float toLightDistance			= glm::length(toLightPosition);
		const glm::vec4 toLightPositionN	= toLightPosition / toLightDistance;

		// Shadow Ray
		const Math::Ray<glm::vec4> shadowRay = Math::Ray<glm::vec4>(result.Position + toLightPositionN * config->SHADOW_START_OFFSET, toLightPositionN);
		result.ShadowValue					 = RayMarchFunctions::MarchSecondaryShadowRay<glm::vec4>(shadowRay, sceneData, toLightDistance, light->Radius, config->MAX_STEPS_SHADOW, config->RAY_HIT_EPSILON, config->SHADOW_PENUMBRA);
	}

	// Color in
	const uchar4 color = isInGroundPlane ? VisualizationHelper::GetColorForRayResult_SimpleLit(*config, result) : VisualizationHelper::GetColorForRayResult(*config, result);
	surf2Dwrite(color, bufferData->SurfaceObject, RESULT_COLOR_COMPONENT_COUNT * sizeof(BufferType) * pixelX, pixelY);
}