#pragma once

#include <glm/ext/vector_float3.hpp>

#include "Rendering/CUDATypes.h"
#include "Rendering/CUDAInterface.h"

#include "Marching/MarchingTypes.h"
#include "Marching/VisualizationHelper.h"

#include "Rendering/Scenes/Scene.h"
#include "MathLib/SignedDistanceFields/SignedDistanceField.h"
#include "Vendor/bitmap_image.hpp"

namespace RayMarchFunctions
{
	static constexpr float MIN_NORMALIZABLE_STEP = 0.000001f;
	static constexpr float NORMAL_EVALUATION_BIAS_Z = 0.0f;
	static constexpr float NORMAL_EVALUATION_BIAS_W = 0.5f;
	static constexpr float ROT_ANGLE_DEG = 8.5f;
	
	//////////////////////////////////////////////////////////////////////////
	
	template <typename N>
	A_CUDA_CPUGPU static RayMarchResult<N> MarchSingleRay(const Math::Ray<N>& ray, const RenderSceneDataCUDA* const renderSceneData, const float maxDistance, const int maxSteps, const float rayHitEpsilon)
	{
		// Closest Position result not supported
		float traversedDistance = 0.0f;
		unsigned short step = 0;
				
		while (true)
		{
			const N position				= ray.At(traversedDistance);
			const auto evaluationDistance = renderSceneData->EvaluateDistance(position);
			
			const bool hitSurface = evaluationDistance < OptionsManager::RAY_HIT_EPSILON;
			if (hitSurface)
			{
				// todo: consider supporting local normals here es well
				return RayMarchResult<N>(true, evaluationDistance, step, traversedDistance, 0, position, renderSceneData->GetLocalSamplePosition(position), position, renderSceneData->EvaluateNormal(position), DimensionVector());
			}

			traversedDistance += evaluationDistance;
			step++;
			
			const bool maximumDistanceReached = traversedDistance > maxDistance;
			if (maximumDistanceReached)
			{
				auto endPosition = ray.At(maxDistance);
				return RayMarchResult<N>(false, renderSceneData->EvaluateDistance(endPosition), traversedDistance, 0, step, position, N(), position);
			}

			const bool maximumStepsReached = step >= maxSteps;
			if (maximumStepsReached)
			{
				auto endPosition = ray.At(maxDistance);
				return RayMarchResult<N>(false, renderSceneData->EvaluateDistance(endPosition), traversedDistance, 0, step, position, N(), position);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	
	template <typename N>
	A_CUDA_CPUGPU static float MarchSecondaryShadowRay(const Math::Ray<N>& ray, const RenderSceneDataCUDA* renderSceneData, float toLightDistance, float lightRadius, int maxSteps, float rayHitEpsilon, float penumbraFactor = 2.0f)
	{
		float resultPenumbra	= 1.0f;
		float traversedDistance = 0.0f;
		unsigned short step		= 0;
				
		while (true)
		{
			const N position		= ray.At(traversedDistance);
			auto evaluationDistance = renderSceneData->EvaluateDistance(position);
			
			if (evaluationDistance < rayHitEpsilon)
			{
				constexpr float DOT_SURFACE = 0.5f;
				float dist;
				const N toSurfaceVector = renderSceneData->EvaluateToSurfaceVectorZW(position, dist);

				const bool hitSurface = glm::dot(toSurfaceVector, ray.Direction) > DOT_SURFACE;
				if (hitSurface)
				{
					return 0.0f;
				}
			}

			evaluationDistance = glm::max(evaluationDistance, rayHitEpsilon);

			const float currentPenumbra	= penumbraFactor * evaluationDistance / (traversedDistance + 0.0001f);
			resultPenumbra = glm::min(resultPenumbra, currentPenumbra);

			traversedDistance += evaluationDistance;
			step++;

			const bool lightDistanceReached = traversedDistance > toLightDistance;
			if (lightDistanceReached)
			{
				// If we reach the light source, we use the distance to the light and the light radius for intensity
				const float intensity = Math::Clamp01(lightRadius / toLightDistance);
				return resultPenumbra * intensity;
			}

			const bool maximumStepsReached = step > maxSteps;
			if (maximumStepsReached)
			{
				// At max steps, we assume that eventually, the light is hit. So the same formula as above is used.
				const float intensity = Math::Clamp01(lightRadius / toLightDistance);
				return resultPenumbra * intensity;
			}
		}
	}
	
	//////////////////////////////////////////////////////////////////////////
	
	template <typename N, typename SpaceTransformationMatrix_t = glm::mat<N::length(), N::length(), float, glm::defaultp>>
	A_CUDA_CPUGPU static RayMarchResult<N> MarchSingleBiRay(const Math::BiRay<N>& biRay, const SpaceTransformationMatrix_t& biRaySpaceToWorldSpace, const RenderSceneDataCUDA* renderSceneData, const float minStepDistance, const float maxDistance, const unsigned int maxSteps, const float rayHitEpsilon)
	{
		// WS = World Space
		// BS = BiRay Space with Origin 0/0
		// TBS = BiRay Space with Origin at ray origin

		using DimVector = N;

		//////////////////////////////////////////////////////////////////////////
				
		const SpaceTransformationMatrix_t worldSpaceToBiRaySpace	= glm::inverse(biRaySpaceToWorldSpace);
		const DimVector biRayOriginBS								= worldSpaceToBiRaySpace * biRay.Origin;

		float traversedDistanceMainTBS	= 0.0f;
		float traversedDistanceSecTBS	= 0.0f;
		glm::vec2 lastStepBS = glm::vec2(0.0f, 0.0f);
		DimVector closestOnRayPositionWS;
		float closestDistanceWS			= 12345.0f;

		unsigned int stepCount		= 0;

		while (true)
		{
			stepCount ++;

			//////////////////////////////////////////////////////////////////////////
			// 1) Asses current position

			// Find current position
			const DimVector rayPositionWS = biRay.At(traversedDistanceMainTBS, traversedDistanceSecTBS);

			// Sample closest surface
			float closestSurfaceDistanceWS;
			const DimVector closestSurfaceVectorWSNormalized = renderSceneData->EvaluateToSurfaceVectorZW(rayPositionWS, closestSurfaceDistanceWS);	

			// Assess closest surface vector
			const DimVector closestSurfacePositionWS = rayPositionWS + closestSurfaceDistanceWS * closestSurfaceVectorWSNormalized;
			
			//////////////////////////////////////////////////////////////////////////

			if (closestSurfaceDistanceWS < closestDistanceWS)
			{
				closestDistanceWS		= closestSurfaceDistanceWS;
				closestOnRayPositionWS	= rayPositionWS;
			}

			//////////////////////////////////////////////////////////////////////////
			// 2) Early Outs

			// Check for hit early out
			const bool hitSurface = closestSurfaceDistanceWS < rayHitEpsilon;
			if (hitSurface)
			{	
				// We hit something! 
				// Now, lets assure that the earliest z is hit by moving along the surface.
				// We move along the edge of the object, and also around corners, as long as we are moving toward a smaller z value.
								
				const DimVector firstHitPositionWS					= closestSurfacePositionWS /*+ moveToSurfacePosition + stepAlongFirstSurfaceHitVectorWS*/;
				float	  firstHitDistanceWS;				
				const DimVector firstHitSurfaceVectorWSNormalized	= renderSceneData->EvaluateToSurfaceVectorZW(firstHitPositionWS, firstHitDistanceWS);
				const DimVector firstHitSurfaceVectorBSNormalized	= worldSpaceToBiRaySpace * firstHitSurfaceVectorWSNormalized;

				//////////////////////////////////////////////////////////////////////////
				// Find vector to move along the surface.
				
				glm::vec2 stepAlongFirstSurfaceHitVectorWS_ZW	= Math::RotateAngle(glm::vec2(firstHitSurfaceVectorWSNormalized.z, firstHitSurfaceVectorWSNormalized.w), glm::radians(90.0f));
				glm::vec2 stepAlongFirstSurfaceHitVectorBS_ZW	= Math::RotateAngle(glm::vec2(firstHitSurfaceVectorBSNormalized.z, firstHitSurfaceVectorBSNormalized.w), glm::radians(90.0f));
				
				const bool movingToEarlyZ = (stepAlongFirstSurfaceHitVectorBS_ZW.x < 0);  
				if (!movingToEarlyZ)
				{
					// Walk along the lower z direction. (x -> z, y -> w).
					stepAlongFirstSurfaceHitVectorWS_ZW		= -stepAlongFirstSurfaceHitVectorWS_ZW;
					stepAlongFirstSurfaceHitVectorBS_ZW		= -stepAlongFirstSurfaceHitVectorBS_ZW;
				}

				stepAlongFirstSurfaceHitVectorWS_ZW			= stepAlongFirstSurfaceHitVectorWS_ZW * minStepDistance;
				DimVector stepAlongSurfaceVectorWS			= DimVector(0, 0, stepAlongFirstSurfaceHitVectorWS_ZW.x, stepAlongFirstSurfaceHitVectorWS_ZW.y);
				
				//////////////////////////////////////////////////////////////////////////
				
				DimVector currentHitPositionWS					= firstHitPositionWS;
				DimVector currentTestPositionWS					= firstHitPositionWS;
				float currentTestDistanceWS						= firstHitDistanceWS;
				DimVector currentTestSurfaceVectorWSNormalized	= firstHitSurfaceVectorWSNormalized;

				int edgeWalkStepCount = 0;
				bool reachedEnd = false;
				while (true)
				{
					const bool finishEdgeWalk = reachedEnd || (currentTestDistanceWS > rayHitEpsilon) || (stepCount >= maxSteps);
					if (finishEdgeWalk)
					{
						currentHitPositionWS						= currentTestPositionWS;

						const DimVector normalEvaluationBias		= DimVector(0.0f, 0.0f, -NORMAL_EVALUATION_BIAS_Z, -NORMAL_EVALUATION_BIAS_W);
						const DimVector currentHitPositionTBS		= worldSpaceToBiRaySpace * currentHitPositionWS - biRayOriginBS;
						traversedDistanceMainTBS					= currentHitPositionTBS.z;
						traversedDistanceSecTBS						= currentHitPositionTBS.w;
	
						const DimVector hitPositionOS				= renderSceneData->GetLocalSamplePosition(currentHitPositionWS);
						const DimVector normalWS					= renderSceneData->EvaluateNormal(currentHitPositionWS + normalEvaluationBias);
						const DimVector floatingHitPositionOS		= renderSceneData->GetLocalSamplePosition(currentHitPositionWS + normalWS);
						const DimVector normalOS					= glm::normalize(floatingHitPositionOS - hitPositionOS);						

						// We are at the end of the edge. This is where we return!
						return RayMarchResult<DimVector>(true, currentTestDistanceWS, stepCount, traversedDistanceMainTBS, traversedDistanceSecTBS, currentHitPositionWS, hitPositionOS, currentHitPositionWS, normalWS, normalOS);
					}

					//////////////////////////////////////////////////////////////////////////
					// Update surface vector

					// Every step, we take the to surface vector and rotate it to get a new move along surface vector.
					// This a) diminishes rounding issues and b) allows us to traverse around several corner points while traversing our edge, stopping at the corner point where the surface vector would point in a positve z direction.
					const DimVector currentSurfaceVectorWS		= currentTestSurfaceVectorWSNormalized;
					const DimVector currentSurfaceVectorBS		= worldSpaceToBiRaySpace * currentTestSurfaceVectorWSNormalized;
				
					const bool positiveW = currentSurfaceVectorBS.w > 0;
					glm::vec2 rotatedVectorWS = Math::RotateAngle(glm::vec2(currentSurfaceVectorWS.z, currentSurfaceVectorWS.w), positiveW ? glm::radians(90.0f) : glm::radians(-90.0f));

					if (rotatedVectorWS.x < 0)
					{
						stepAlongSurfaceVectorWS = DimVector(0, 0, rotatedVectorWS.x * minStepDistance, rotatedVectorWS.y * minStepDistance);
					}
					else
					{
						reachedEnd = true;
						continue;
					}
					
					//////////////////////////////////////////////////////////////////////////
					// Step along the edge	
					
					stepCount ++;
					edgeWalkStepCount ++;

					currentHitPositionWS					= currentTestPositionWS;
					currentTestPositionWS					= currentHitPositionWS + stepAlongSurfaceVectorWS;
					currentTestSurfaceVectorWSNormalized	= renderSceneData->EvaluateToSurfaceVectorZW(currentTestPositionWS, currentTestDistanceWS);	

					continue;
				}
			}
			
			const bool maxDistanceReached = traversedDistanceMainTBS > maxDistance;
			if (maxDistanceReached)
			{
				// No hit - maximum distance
				break;
			}

			const bool maxStepsReached = stepCount >= maxSteps;
			if (maxStepsReached)
			{
				// No hit - maximum steps
				break;
			}

			//////////////////////////////////////////////////////////////////////////
			// 3) Calculate next step direction
			
			const DimVector closestSurfacePositionBS = worldSpaceToBiRaySpace * closestSurfacePositionWS;
			
			// Step along fractions
			// If both fractions are small but the total distance is bigger, that means that we are off in either the ray right or right up axis.
			// In that case, we step along the forward axis.
			const float toClosestMainTBS				= glm::abs(closestSurfacePositionBS.z - biRayOriginBS.z);
			const float toClosestSecondaryTBS			= closestSurfacePositionBS.w - biRayOriginBS.w;
			
			const float toClosestMainBSDelta			= glm::abs(toClosestMainTBS - traversedDistanceMainTBS);
			const float toClosestSecondaryBSDelta		= (toClosestSecondaryTBS - traversedDistanceSecTBS);
			
			//////////////////////////////////////////////////////////////////////////
			// Calculate next step movement cone

			glm::vec2 moveVectorConeMiddleBS = {toClosestMainBSDelta, toClosestSecondaryBSDelta};
			const float moveVectorConeMiddeleSQL	= Math::LengthSquared(moveVectorConeMiddleBS);
			if (moveVectorConeMiddeleSQL < MIN_NORMALIZABLE_STEP)
			{
				moveVectorConeMiddleBS = glm::normalize(lastStepBS) * minStepDistance;
			}
			else if (moveVectorConeMiddeleSQL < minStepDistance * minStepDistance)
			{
				moveVectorConeMiddleBS = glm::normalize(moveVectorConeMiddleBS) * minStepDistance;
			}
			else
			{
				lastStepBS = moveVectorConeMiddleBS;
			}

			lastStepBS = moveVectorConeMiddleBS;

			
			// The cone works like this:
			//            ,L
			//        ,--´
			//   ,--´
			// O-----------M
			//   `--.
			//       `--.
			//            `R
			//
			// L and R are the result of rotating M and then making them align on the vector perpendicular to M.
			// This is achieved by dividing the rotated vector Lproj by cos(a): L = Lproj / cos(a) = Lproj / (|M| / |Lproj|) 

			const glm::vec2 moveVectorConeLeftBS = Math::RotateAngle(moveVectorConeMiddleBS, glm::radians(-ROT_ANGLE_DEG)) / glm::cos(glm::radians(-ROT_ANGLE_DEG));
			const glm::vec2 moveVectorConeRightBS = Math::RotateAngle(moveVectorConeMiddleBS, glm::radians(ROT_ANGLE_DEG)) / glm::cos(glm::radians(ROT_ANGLE_DEG));

			// Check Cone Distances
			
			const DimVector positionConeLeft	= biRay.At(traversedDistanceMainTBS + moveVectorConeLeftBS.x,	traversedDistanceSecTBS + moveVectorConeLeftBS.y);
			const DimVector positionConeMiddle	= biRay.At(traversedDistanceMainTBS + moveVectorConeMiddleBS.x,	traversedDistanceSecTBS + moveVectorConeMiddleBS.y);
			const DimVector positionConeRight	= biRay.At(traversedDistanceMainTBS + moveVectorConeRightBS.x,	traversedDistanceSecTBS + moveVectorConeRightBS.y);
			
			const float distanceConeLeft	= renderSceneData->EvaluateDistance(positionConeLeft);
			const float distanceConeMiddle	= renderSceneData->EvaluateDistance(positionConeMiddle);
			const float distanceConeRight	= renderSceneData->EvaluateDistance(positionConeRight);

			////////////////////////////////////////////////////////////////////////
			// 4) Find best step

			const bool isLeftClosest	= distanceConeLeft < distanceConeMiddle && distanceConeLeft < distanceConeRight;
			const bool isMiddleClosest	= distanceConeMiddle < distanceConeRight && !isLeftClosest;
			//bool isRightClosest		= !isLeftClosest && !isMiddleClosest;

			////////////////////////////////////////////////////////////////////////
			// 5) Perform Step

			if (isLeftClosest)
			{
				traversedDistanceMainTBS	+= moveVectorConeLeftBS.x;
				traversedDistanceSecTBS		+= moveVectorConeLeftBS.y;
			}
			else if (isMiddleClosest)
			{
				traversedDistanceMainTBS	+= moveVectorConeMiddleBS.x;
				traversedDistanceSecTBS		+= moveVectorConeMiddleBS.y;
			}
			else
			{
				traversedDistanceMainTBS	+= moveVectorConeRightBS.x;
				traversedDistanceSecTBS		+= moveVectorConeRightBS.y;
			}
		}

		// RESULT

		const DimVector endPosition	= biRay.At(traversedDistanceMainTBS, traversedDistanceSecTBS);
		const float distanceWS		= renderSceneData->EvaluateDistance(endPosition);
		return RayMarchResult<DimVector>(false, distanceWS, stepCount, traversedDistanceMainTBS, traversedDistanceSecTBS, endPosition, DimVector(), closestOnRayPositionWS);
	}

	//////////////////////////////////////////////////////////////////////////

	#pragma optimize( "", off )
	// Note that this debug implementation does not support templates anymore, as static templates can not be individually marked as "do not optimize".
	static RayMarchResult<glm::vec4> MarchSingleBiRay_DEBUG(const Math::BiRay<glm::vec4>& biRay, const glm::mat<4, 4, float, glm::defaultp>& biRaySpaceToWorldSpace, 
		const RenderSceneDataCUDA* const renderSceneData, const float minStepDistance, const float maxDistance, const unsigned int maxSteps, const float rayHitEpsilon, 
		const float dStartMain, const float dStartSec, const float dStepSize, const float dGridSize, bitmap_image& inOutImagePath, bitmap_image& inOutImageCombined)
	{
		// WS = World Space
		// BS = BiRay Space with Origin 0/0
		// TBS = BiRay Space with Origin at ray origin
		
		using DimVector = glm::vec4;
		using SpaceTransformationMatrix_t = glm::mat<4, 4, float, glm::defaultp>;
		//constexpr float ROT_ANGLE_DEG = 20.0f;

		//////////////////////////////////////////////////////////////////////////
		// DEBUG

		const auto ToPixelPosition = [dStartMain, dStartSec, dStepSize](const glm::vec2& positionLocalZW) 
		{
			return glm::ivec2((int) std::round((positionLocalZW.x - dStartMain) / dStepSize), (int) std::round((positionLocalZW.y - dStartSec) / dStepSize));
		};
		
		const auto ToPixelVector = [dStepSize](const glm::vec2& positionLocalZW) 
		{
			return glm::ivec2((int) std::round((positionLocalZW.x) / dStepSize), (int) std::round((positionLocalZW.y) / dStepSize));
		};

		const auto ToPixelScalar = [dStepSize](const float& scalar) 
		{
			return (int) (scalar / dStepSize);
		};

		//////////////////////////////////////////////////////////////////////////

		const SpaceTransformationMatrix_t worldSpaceToBiRaySpace	= glm::inverse(biRaySpaceToWorldSpace);
		const DimVector biRayOriginBS								= worldSpaceToBiRaySpace * biRay.Origin;

		float traversedDistanceMainTBS	= 0.0f;
		float traversedDistanceSecTBS	= 0.0f;
		glm::vec2 lastStepBS = glm::vec2(0.0f, 0.0f);
		DimVector closestOnRayPositionWS;
		float closestDistanceWS			= 12345.0f;

		unsigned int stepCount		= 0;
		
		image_drawer drawerPath(inOutImagePath);
		image_drawer drawerCombined(inOutImageCombined);

		while (true)
		{
			stepCount ++;

			//////////////////////////////////////////////////////////////////////////
			// 1) Asses current position

			// Find current position
			const DimVector rayPositionWS						= biRay.At(traversedDistanceMainTBS, traversedDistanceSecTBS);

			// Sample closest surface
			float closestSurfaceDistanceWS;
			const DimVector closestSurfaceVectorWSNormalized	= renderSceneData->EvaluateToSurfaceVectorZW(rayPositionWS, closestSurfaceDistanceWS);	

			// Assess closest surface vector
			const DimVector closestSurfacePositionWS			= rayPositionWS + closestSurfaceDistanceWS * closestSurfaceVectorWSNormalized;
			
			//////////////////////////////////////////////////////////////////////////

			if (closestSurfaceDistanceWS < closestDistanceWS)
			{
				closestDistanceWS		= closestSurfaceDistanceWS;
				closestOnRayPositionWS	= rayPositionWS;
			}

			//////////////////////////////////////////////////////////////////////////
			// 2) Early Outs

			// Check for hit early out
			const bool hitSurface = closestSurfaceDistanceWS < rayHitEpsilon;
			if (hitSurface)
			{	
				// We hit something! 
				// Now, lets assure that the earliest z is hit by moving along the surface.
				// We move along the edge of the object, and also around corners, as long as we are moving toward a smaller z value.
								
				const DimVector firstHitPositionWS					= closestSurfacePositionWS;
				float firstHitDistanceWS;				
				const DimVector firstHitSurfaceVectorWSNormalized	= renderSceneData->EvaluateToSurfaceVectorZW(firstHitPositionWS, firstHitDistanceWS);
				const DimVector firstHitSurfaceVectorBSNormalized	= worldSpaceToBiRaySpace * firstHitSurfaceVectorWSNormalized;

				//////////////////////////////////////////////////////////////////////////
				// Find vector to move along the surface.
				
				glm::vec2 stepAlongFirstSurfaceHitVectorWS_ZW	= Math::RotateAngle(glm::vec2(firstHitSurfaceVectorWSNormalized.z, firstHitSurfaceVectorWSNormalized.w), glm::radians(90.0f));
				glm::vec2 stepAlongFirstSurfaceHitVectorBS_ZW	= Math::RotateAngle(glm::vec2(firstHitSurfaceVectorBSNormalized.z, firstHitSurfaceVectorBSNormalized.w), glm::radians(90.0f));
				
				bool movingToEarlyZ = (stepAlongFirstSurfaceHitVectorBS_ZW.x < 0);  
				if (!movingToEarlyZ)
				{
					// Walk along the lower z direction. (x -> z, y -> w).
					stepAlongFirstSurfaceHitVectorWS_ZW		= -stepAlongFirstSurfaceHitVectorWS_ZW;
					stepAlongFirstSurfaceHitVectorBS_ZW		= -stepAlongFirstSurfaceHitVectorBS_ZW;
				}

				stepAlongFirstSurfaceHitVectorWS_ZW			= stepAlongFirstSurfaceHitVectorWS_ZW * minStepDistance;
				DimVector stepAlongSurfaceVectorWS			= DimVector(0, 0, stepAlongFirstSurfaceHitVectorWS_ZW.x, stepAlongFirstSurfaceHitVectorWS_ZW.y);
				
				//////////////////////////////////////////////////////////////////////////
				
				DimVector currentHitPositionWS					= firstHitPositionWS;
				DimVector currentTestPositionWS					= firstHitPositionWS;
				float currentTestDistanceWS						= firstHitDistanceWS;
				DimVector currentTestSurfaceVectorWSNormalized	= firstHitSurfaceVectorWSNormalized;

				int edgeWalkStepCount = 0;
				bool reachedEnd = false;
				while (true)
				{
					const bool finishEdgeWalk = reachedEnd || (currentTestDistanceWS > rayHitEpsilon) || (stepCount >= maxSteps);
					if (finishEdgeWalk)
					{
						currentHitPositionWS = currentTestPositionWS;

						const DimVector normalEvaluationBias		= DimVector(0.0f, 0.0f, -NORMAL_EVALUATION_BIAS_Z, -NORMAL_EVALUATION_BIAS_W);

						//////////////////////////////////////////////////////////////////////////
						// DEBUG DRAWING

						DimVector finalPositionTBS						= worldSpaceToBiRaySpace * currentTestPositionWS - biRayOriginBS;
						glm::vec2 pxFinalPosition						= ToPixelPosition(glm::vec2(finalPositionTBS.z, finalPositionTBS.w));

						constexpr float NORMAL_DRAW_LENGTH = 1.0f;

						DimVector finalNormalWS							= renderSceneData->EvaluateNormal(currentHitPositionWS);
						DimVector finalNormalBS							= worldSpaceToBiRaySpace * finalNormalWS;
						glm::vec2 finalPositionWithNormalTBS			= glm::vec2(finalPositionTBS.z + NORMAL_DRAW_LENGTH * finalNormalBS.z, finalPositionTBS.w + NORMAL_DRAW_LENGTH * finalNormalBS.w);
						glm::vec2 pxFinalPositionWithNormal				= ToPixelPosition(finalPositionWithNormalTBS);

						DimVector finalNormalBiasedWS					= renderSceneData->EvaluateNormal(currentHitPositionWS + normalEvaluationBias);
						DimVector finalNormalBiasedBS					= worldSpaceToBiRaySpace * finalNormalBiasedWS;
						glm::vec2 finalPositionWithNormalBiasedTBS		= glm::vec2(finalPositionTBS.z + NORMAL_DRAW_LENGTH * finalNormalBiasedBS.z, finalPositionTBS.w + NORMAL_DRAW_LENGTH * finalNormalBiasedBS.w);
						glm::vec2 pxFinalPositionWithBiasedNormal		= ToPixelPosition(finalPositionWithNormalBiasedTBS);
						
						drawerPath.pen_color(255, 255, 120);
						drawerCombined.pen_color(255, 255, 120);
						drawerCombined.pen_width(2);
						
						drawerPath.line_segment((int) pxFinalPosition.x, (int) pxFinalPosition.y, (int) pxFinalPositionWithNormal.x, (int) pxFinalPositionWithNormal.y);
						drawerCombined.line_segment((int) pxFinalPosition.x, (int) pxFinalPosition.y, (int) pxFinalPositionWithNormal.x, (int) pxFinalPositionWithNormal.y);
						drawerCombined.line_segment((int) pxFinalPosition.x, (int) pxFinalPosition.y + (int) dGridSize, (int) pxFinalPositionWithNormal.x, (int) pxFinalPositionWithNormal.y + (int) dGridSize);
						
						drawerPath.pen_color(100, 200, 220);
						drawerCombined.pen_color(100, 200, 220);
						drawerCombined.pen_width(3);

						drawerPath.line_segment((int) pxFinalPosition.x, (int) pxFinalPosition.y, (int) pxFinalPositionWithBiasedNormal.x, (int) pxFinalPositionWithBiasedNormal.y);
						drawerCombined.line_segment((int) pxFinalPosition.x, (int) pxFinalPosition.y, (int) pxFinalPositionWithBiasedNormal.x, (int) pxFinalPositionWithBiasedNormal.y);
						drawerCombined.line_segment((int) pxFinalPosition.x, (int) pxFinalPosition.y + (int) dGridSize, (int) pxFinalPositionWithBiasedNormal.x, (int) pxFinalPositionWithBiasedNormal.y + (int) dGridSize);
						
						drawerPath.pen_color(150, 50, 220);
						drawerCombined.pen_color(150, 50, 220);

						drawerPath.circle((int) pxFinalPosition.x, (int) pxFinalPosition.y, 1);
						drawerCombined.circle((int) pxFinalPosition.x, (int) pxFinalPosition.y, 1);
						drawerCombined.circle((int) pxFinalPosition.x, (int) pxFinalPosition.y + (int) dGridSize, 1);

						// END DEBUG DRAWING
						//////////////////////////////////////////////////////////////////////////

						const DimVector currentHitPositionTBS = worldSpaceToBiRaySpace * currentHitPositionWS - biRayOriginBS;
						traversedDistanceMainTBS		= currentHitPositionTBS.z;
						traversedDistanceSecTBS			= currentHitPositionTBS.w;
	
						const DimVector hitPositionOS				= renderSceneData->GetLocalSamplePosition(currentHitPositionWS);
						const DimVector normalWS					= renderSceneData->EvaluateNormal(currentHitPositionWS + normalEvaluationBias);
						const DimVector floatingHitPositionOS		= renderSceneData->GetLocalSamplePosition(currentHitPositionWS + normalWS);
						const DimVector normalOS					= glm::normalize(floatingHitPositionOS - hitPositionOS);						

						//printf("Normal WS %f, %f, %f, %f | OS %f, %f, %f, %f \n", normalWS.x, normalWS.y, normalWS.z, normalWS.w, normalOS.x, normalOS.y, normalOS.z, normalOS.w);

						// We are at the end of the edge. This is where we return!
						return RayMarchResult<DimVector>(true, currentTestDistanceWS, stepCount, traversedDistanceMainTBS, traversedDistanceSecTBS, currentHitPositionWS, hitPositionOS, currentHitPositionWS, normalWS, normalOS);
					}

					//////////////////////////////////////////////////////////////////////////
					// Update surface vector

					// Every step, we take the to surface vector and rotate it to get a new move along surface vector.
					// This a) diminishes rounding issues and b) allows us to traverse around several corner points while traversing our edge, stopping at the corner point where the surface vector would point in a positve z direction.
					const DimVector currentSurfaceVectorWS		= currentTestSurfaceVectorWSNormalized;
					const DimVector currentSurfaceVectorBS		= worldSpaceToBiRaySpace * currentTestSurfaceVectorWSNormalized;
				
					const bool positiveW = currentSurfaceVectorBS.w > 0;
					const glm::vec2 rotatedVectorWS = Math::RotateAngle(glm::vec2(currentSurfaceVectorWS.z, currentSurfaceVectorWS.w), positiveW ? glm::radians(90.0f) : glm::radians(-90.0f));

					if (rotatedVectorWS.x < 0)
					{
						stepAlongSurfaceVectorWS = DimVector(0, 0, rotatedVectorWS.x * minStepDistance, rotatedVectorWS.y * minStepDistance);
					}
					else
					{
						reachedEnd = true;
						continue;
					}
					
					//////////////////////////////////////////////////////////////////////////
					// DEBUG DRAWING
				
					const DimVector testHitPositionTBS				= worldSpaceToBiRaySpace * currentTestPositionWS - biRayOriginBS;
					const DimVector testHitPositionWithNormalTBS		= worldSpaceToBiRaySpace * (currentTestPositionWS - currentTestSurfaceVectorWSNormalized) - biRayOriginBS;
					const glm::vec2 pxTestHitPosition					= ToPixelPosition(glm::vec2(testHitPositionTBS.z, testHitPositionTBS.w));
					const glm::vec2 pxTestPositionWithNormal			= ToPixelPosition(glm::vec2(testHitPositionWithNormalTBS.z, testHitPositionWithNormalTBS.w));
				
					drawerPath.pen_color(90, 120, 20);
					drawerCombined.pen_color(90, 120, 20);
					
					if (edgeWalkStepCount == 0)
					{
						drawerPath.pen_width(2);
						drawerCombined.pen_width(2);
					}

					drawerPath.line_segment((int) pxTestHitPosition.x, (int) pxTestHitPosition.y, (int) pxTestPositionWithNormal.x, (int) pxTestPositionWithNormal.y);
					drawerCombined.line_segment((int) pxTestHitPosition.x, (int) pxTestHitPosition.y, (int) pxTestPositionWithNormal.x, (int) pxTestPositionWithNormal.y);
					drawerCombined.line_segment((int) pxTestHitPosition.x, (int) pxTestHitPosition.y + (int) dGridSize, (int) pxTestPositionWithNormal.x, (int) pxTestPositionWithNormal.y + (int) dGridSize);
					
					drawerPath.pen_width(1);
					drawerCombined.pen_width(1);	
					
					const DimVector beforePositionTBS		= worldSpaceToBiRaySpace * currentHitPositionWS - biRayOriginBS;
					const DimVector afterPositionTBS		= worldSpaceToBiRaySpace * currentTestPositionWS - biRayOriginBS;
					const glm::vec2 pxPosBefore			= ToPixelPosition(glm::vec2(beforePositionTBS.z, beforePositionTBS.w));
					const glm::vec2 pxPosAfter			= ToPixelPosition(glm::vec2(afterPositionTBS.z, afterPositionTBS.w));
			
					drawerPath.pen_width(1);
					drawerPath.pen_color(80, 80, 80);
					drawerCombined.pen_width(1);
					drawerCombined.pen_color(200, 200, 200);

					// Draw path for both outputs
					drawerPath.line_segment((int) pxPosBefore.x, (int) pxPosBefore.y, (int) pxPosAfter.x, (int) pxPosAfter.y);
					drawerCombined.line_segment((int) pxPosBefore.x, (int) pxPosBefore.y, (int) pxPosAfter.x, (int) pxPosAfter.y);
					drawerCombined.line_segment((int) pxPosBefore.x, (int) pxPosBefore.y + (int) dGridSize, (int) pxPosAfter.x, (int) pxPosAfter.y + (int) dGridSize);
					
					// Draw steps that were taken
					drawerPath.pen_color(255, 255, 255);
					drawerPath.circle((int) pxPosBefore.x, (int) pxPosBefore.y, 2);
					drawerCombined.circle((int) pxPosBefore.x, (int) pxPosBefore.y, 2);
					drawerCombined.circle((int) pxPosBefore.x, (int) pxPosBefore.y + (int) dGridSize, 2);
					
					// END DEBUG DRAWING
					//////////////////////////////////////////////////////////////////////////
					
					//////////////////////////////////////////////////////////////////////////
					// Step along the edge					
					stepCount ++;
					edgeWalkStepCount ++;

					currentHitPositionWS			= currentTestPositionWS;
					currentTestPositionWS					= currentHitPositionWS + stepAlongSurfaceVectorWS;
					currentTestSurfaceVectorWSNormalized	= renderSceneData->EvaluateToSurfaceVectorZW(currentTestPositionWS, currentTestDistanceWS);	

					continue;
				}
			}
			
			const bool maxDistanceReached = traversedDistanceMainTBS > maxDistance;
			if (maxDistanceReached)
			{
				// No hit - maximum distance
				break;
			}

			const bool maxStepsReached = stepCount >= maxSteps;
			if (maxStepsReached)
			{
				// No hit - maximum steps
				break;
			}

			//////////////////////////////////////////////////////////////////////////
			// 3) Calculate next step direction
			
			const DimVector closestSurfacePositionBS = worldSpaceToBiRaySpace * closestSurfacePositionWS;
			
			// Step along fractions
			// If both fractions are small but the total distance is bigger, that means that we are off in either the ray right or right up axis.
			// In that case, we step along the froward axis.
			const float toClosestMainTBS				= glm::abs(closestSurfacePositionBS.z - biRayOriginBS.z);
			const float toClosestSecondaryTBS			= /*glm::abs(*/closestSurfacePositionBS.w - biRayOriginBS.w;
			
			const float toClosestMainBSDelta			= glm::abs(toClosestMainTBS - traversedDistanceMainTBS);
			const float toClosestSecondaryBSDelta		= (toClosestSecondaryTBS - traversedDistanceSecTBS);
			
			//////////////////////////////////////////////////////////////////////////
			// Calculate next step movement cone

			glm::vec2 moveVectorConeMiddleBS		= {toClosestMainBSDelta, toClosestSecondaryBSDelta};
			const float moveVectorConeMiddeleSQL	= Math::LengthSquared(moveVectorConeMiddleBS);
			if (moveVectorConeMiddeleSQL < MIN_NORMALIZABLE_STEP)
			{
				moveVectorConeMiddleBS = glm::normalize(lastStepBS) * minStepDistance;
			}
			else if (moveVectorConeMiddeleSQL < minStepDistance * minStepDistance)
			{
				moveVectorConeMiddleBS = glm::normalize(moveVectorConeMiddleBS) * minStepDistance;
			}
			else
			{
				lastStepBS = moveVectorConeMiddleBS;
			}

			lastStepBS = moveVectorConeMiddleBS;

			
			// The cone works like this:
			//            ,L
			//        ,--´
			//   ,--´
			// O-----------M
			//   `--.
			//       `--.
			//            `R
			//
			// L and R are the result of rotating M and then making them align on the vector perpendicualr to M.
			// This is achieved by dividing the rotated vector Lproj by cos(a): L = Lproj / cos(a) = Lproj / (|M| / |Lproj|) 

			const glm::vec2 moveVectorConeLeftBS = Math::RotateAngle(moveVectorConeMiddleBS, glm::radians(-ROT_ANGLE_DEG)) / glm::cos(glm::radians(-ROT_ANGLE_DEG));
			const glm::vec2 moveVectorConeRightBS = Math::RotateAngle(moveVectorConeMiddleBS, glm::radians(ROT_ANGLE_DEG)) / glm::cos(glm::radians(ROT_ANGLE_DEG));

			// Check Cone Distances
			
			const DimVector positionConeLeft	= biRay.At(traversedDistanceMainTBS + moveVectorConeLeftBS.x,		traversedDistanceSecTBS + moveVectorConeLeftBS.y);
			const DimVector positionConeMiddle	= biRay.At(traversedDistanceMainTBS + moveVectorConeMiddleBS.x,		traversedDistanceSecTBS + moveVectorConeMiddleBS.y);
			const DimVector positionConeRight	= biRay.At(traversedDistanceMainTBS + moveVectorConeRightBS.x,		traversedDistanceSecTBS + moveVectorConeRightBS.y);
			
			const float distanceConeLeft		= renderSceneData->EvaluateDistance(positionConeLeft);
			const float distanceConeMiddle		= renderSceneData->EvaluateDistance(positionConeMiddle);
			const float distanceConeRight		= renderSceneData->EvaluateDistance(positionConeRight);
			
			////////////////////////////////////////////////////////////////////////
			// 4) Find best step

			const bool isLeftClosest	= distanceConeLeft < distanceConeMiddle && distanceConeLeft < distanceConeRight;
			const bool isMiddleClosest	= distanceConeMiddle < distanceConeRight && !isLeftClosest;
			//bool isRightClosest		= !isLeftClosest && !isMiddleClosest;

			////////////////////////////////////////////////////////////////////////
			// DEBUG VARIABLE

			const glm::vec2 pxPosBefore = ToPixelPosition(glm::vec2(traversedDistanceMainTBS, traversedDistanceSecTBS));

			////////////////////////////////////////////////////////////////////////
			// 5) Perform Step

			if (isLeftClosest)
			{
				traversedDistanceMainTBS	+= moveVectorConeLeftBS.x;
				traversedDistanceSecTBS		+= moveVectorConeLeftBS.y;
			}
			else if (isMiddleClosest)
			{
				traversedDistanceMainTBS	+= moveVectorConeMiddleBS.x;
				traversedDistanceSecTBS		+= moveVectorConeMiddleBS.y;
			}
			else
			{
				traversedDistanceMainTBS	+= moveVectorConeRightBS.x;
				traversedDistanceSecTBS		+= moveVectorConeRightBS.y;
			}

			//////////////////////////////////////////////////////////////////////////
			// DEBUG DRAWING

			const glm::vec2 pxPosAfter = ToPixelPosition(glm::vec2(traversedDistanceMainTBS, traversedDistanceSecTBS));
			
			drawerCombined.pen_width(1);
			drawerPath.pen_width(1);
			drawerPath.pen_color(50, 50, 50);

			// Draw path for both outputs
			drawerPath.line_segment((int) pxPosBefore.x, (int) pxPosBefore.y, (int) pxPosAfter.x, (int) pxPosAfter.y);
			drawerCombined.line_segment((int) pxPosBefore.x, (int) pxPosBefore.y, (int) pxPosAfter.x, (int) pxPosAfter.y);
			drawerCombined.line_segment((int) pxPosBefore.x, (int) pxPosBefore.y + (int) dGridSize, (int) pxPosAfter.x, (int) pxPosAfter.y + (int) dGridSize);
			
			// Draw steps that were taken
			drawerPath.pen_color(255, 255, 255);
			drawerPath.circle((int) pxPosBefore.x, (int) pxPosBefore.y, 2);
			drawerCombined.circle((int) pxPosBefore.x, (int) pxPosBefore.y, 2);
			drawerCombined.circle((int) pxPosBefore.x, (int) pxPosBefore.y + (int) dGridSize, 2);

			// Draw cone for PATH output
			const unsigned char stepCountColor = static_cast<unsigned char>(stepCount / 60.0f * 255);
			drawerPath.pen_color(stepCountColor / 2, stepCountColor / 2, stepCountColor);
			const glm::ivec2 pixelMoveVectorLeft		= ToPixelVector(moveVectorConeLeftBS);
			const glm::ivec2 pixelMoveVectorMiddle	= ToPixelVector(moveVectorConeMiddleBS);
			const glm::ivec2 pixelMoveVectorRight		= ToPixelVector(moveVectorConeRightBS);
				
			if (pxPosAfter.x >= 0 && pxPosAfter.x <= dGridSize && pxPosAfter.y >= 0 && pxPosAfter.y <= dGridSize)
			{
				if (isLeftClosest)
				{
					drawerPath.pen_color(255, 0, 0);
					drawerPath.circle((int) pxPosBefore.x + (int) pixelMoveVectorLeft.x,	(int) pxPosBefore.y + (int) pixelMoveVectorLeft.y,	ToPixelScalar(distanceConeLeft));
				}
				else if (isMiddleClosest)
				{
					drawerPath.pen_color(0, 255, 0);
					drawerPath.circle((int) pxPosBefore.x + (int) pixelMoveVectorMiddle.x,	(int) pxPosBefore.y + (int) pixelMoveVectorMiddle.y, ToPixelScalar(closestSurfaceDistanceWS /*distanceConeMiddle*/));
				}
				else
				{
					drawerPath.pen_color(0, 0, 255);
					drawerPath.circle((int) pxPosBefore.x + (int) pixelMoveVectorRight.x,	(int) pxPosBefore.y + (int) pixelMoveVectorRight.y,	ToPixelScalar(distanceConeRight));
				}
			}

			drawerPath.pen_color(50, 50, 50);
				
			if (pxPosBefore.x + pixelMoveVectorLeft.x >= 0		&& pxPosBefore.x + pixelMoveVectorLeft.x <= dGridSize	&& pxPosBefore.y + pixelMoveVectorLeft.y >= 0	&& pxPosBefore.y + pixelMoveVectorLeft.y <= dGridSize	&& 
				pxPosBefore.x + pixelMoveVectorMiddle.x >= 0	&& pxPosBefore.x + pixelMoveVectorMiddle.x <= dGridSize && pxPosBefore.y + pixelMoveVectorMiddle.y >= 0 && pxPosBefore.y + pixelMoveVectorMiddle.y <= dGridSize && 
				pxPosBefore.x + pixelMoveVectorRight.x >= 0		&& pxPosBefore.x + pixelMoveVectorRight.x <= dGridSize	&& pxPosBefore.y + pixelMoveVectorRight.y >= 0	&& pxPosBefore.y + pixelMoveVectorRight.y <= dGridSize )
			{
				drawerPath.line_segment((int) pxPosBefore.x + (int) pixelMoveVectorLeft.x,		(int) pxPosBefore.y + (int) pixelMoveVectorLeft.y,		(int) pxPosBefore.x + (int) pixelMoveVectorMiddle.x,	(int) pxPosBefore.y + (int) pixelMoveVectorMiddle.y);
				drawerPath.line_segment((int) pxPosBefore.x + (int) pixelMoveVectorMiddle.x,	(int) pxPosBefore.y + (int) pixelMoveVectorMiddle.y,	(int) pxPosBefore.x + (int) pixelMoveVectorRight.x,		(int) pxPosBefore.y + (int) pixelMoveVectorRight.y);
				
				drawerPath.pen_color(250, 250, 250);
				drawerPath.circle((int) pxPosBefore.x + (int) pixelMoveVectorLeft.x,	(int) pxPosBefore.y + (int) pixelMoveVectorLeft.y,	 2);
				drawerPath.circle((int) pxPosBefore.x + (int) pixelMoveVectorMiddle.x,	(int) pxPosBefore.y + (int) pixelMoveVectorMiddle.y, 2);
				drawerPath.circle((int) pxPosBefore.x + (int) pixelMoveVectorRight.x,	(int) pxPosBefore.y + (int) pixelMoveVectorRight.y,	 2);
			}

			// Draw step for COMBINED output			
			drawerCombined.pen_color(170, 255, 200);
			if (pxPosBefore.x >= 0 && pxPosBefore.x <= dGridSize && pxPosBefore.y >= 0 && pxPosBefore.y <= dGridSize)
			{
				drawerCombined.circle((int) pxPosBefore.x, (int) pxPosBefore.y, 1);
				drawerCombined.circle((int) pxPosBefore.x, (int) pxPosBefore.y, ToPixelScalar(closestSurfaceDistanceWS));
				drawerCombined.circle((int) pxPosBefore.x, (int) pxPosBefore.y + (int) dGridSize, 1);
				drawerCombined.circle((int) pxPosBefore.x, (int) pxPosBefore.y + (int) dGridSize, ToPixelScalar(closestSurfaceDistanceWS));
			}
		}

		// RESULT

		const DimVector endPosition	= biRay.At(traversedDistanceMainTBS, traversedDistanceSecTBS);
		const float distanceWS		= renderSceneData->EvaluateDistance(endPosition);
		return RayMarchResult<DimVector>(false, distanceWS, stepCount, traversedDistanceMainTBS, traversedDistanceSecTBS, endPosition, DimVector(), closestOnRayPositionWS);
	}
	#pragma optimize( "", on ) 
}