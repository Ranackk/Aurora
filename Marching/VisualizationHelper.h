#pragma once

#include "MathLib/Mathlib.h"
#include "Options/OptionsManager.h"
#include "Options/Configuration.h"
#include "Marching/MarchingTypes.h"
#include "Rendering/CUDATypes.h"

class VisualizationHelper
{
public:
	VisualizationHelper() = delete;

	template<typename N>
	A_CUDA_CPUGPU static ResultColor GetColorForRayResult(const Configuration& config, const RayMarchResult<N>& result)
	{
		switch (result.Hit ? config.DrawModeHit : config.DrawModeMiss)
		{
			case Configuration::DrawMode::Shaded:				return GetColorForRayResult_Shaded(config, result);
			case Configuration::DrawMode::ShadedNoFog:			return GetColorForRayResult_ShadedNoFog(config, result);
			case Configuration::DrawMode::SimpleLit:			return GetColorForRayResult_SimpleLit(config, result);
			case Configuration::DrawMode::SimpleLitSurface:		return GetColorForRayResult_SimpleLitSurface(config, result);
			case Configuration::DrawMode::NormalFast:			return GetColorForRayResult_Normal(config, result);
			case Configuration::DrawMode::NormalConfigurable:	return GetColorForRayResult_NormalConfigurable(config, result);
			case Configuration::DrawMode::Steps:				return GetColorForRayResult_Steps(config, result);
			case Configuration::DrawMode::TraversedPrimary:		return GetColorForRayResult_TraversedPrimary(config, result);
			case Configuration::DrawMode::TraversedSecondary:	return GetColorForRayResult_TraversedSecondary(config, result);
			case Configuration::DrawMode::SolidColor:			return GetColorForRayResult_SolidColor(config, result);
			case Configuration::DrawMode::Position:				return GetColorForRayResult_Position(config, result);
			case Configuration::DrawMode::LocalPosition:		return GetColorForRayResult_LocalPosition(config, result);
			case Configuration::DrawMode::DistanceToSurface:	return GetColorForRayResult_DistanceToSureface(config, result);
			case Configuration::DrawMode::Transparent:			return ResultColor{0, 0, 0, 0};
			case Configuration::DrawMode::CloseToGeo:			return GetColorForRayResult_CloseToGeo(config, result);
			case Configuration::DrawMode::Checker:				return GetColorForRayResult_Checker(config, result);
			case Configuration::DrawMode::ColoredChecker:		return GetColorForRayResult_ColoredChecker(config, result);
			case Configuration::DrawMode::Surfaces:				return GetColorForRayResult_Surfaces(config, result);
			case Configuration::DrawMode::W_Heat:				return GetColorForRayResult_W_Heat(config, result);
		}

		return ResultColor{255, 0, 255, 255};
	}

//////////////////////////////////////////////////////////////////////////

public:
	
//////////////////////////////////////////////////////////////////////////

template<typename N>
A_CUDA_CPUGPU static ResultColor GetColorForRayResult_SimpleLit(const Configuration& config, const RayMarchResult<N>& result)
{
	if (!result.Hit)
	{
		return GetColorForRayResult_SolidColor(config, result);
	}

	// Shade
	ResultColor shadedColor = ResultColor{255, 255, 255, 255} * Math::Clamp01(Math::Remap(0.0f, 1.0f, config.AMBIENT_LIGHT_AMOUNT, 1.0f, result.ShadowValue));

	// Depth Fog		
	const float depth					= Math::Clamp01(Math::Remap(-10.0f, 40.0f, 0.0f, 1.0f, result.Position.z + result.Position.w));
	const ResultColor depthFogColorBlue = ResultColor{20, 20, 35, 255};
	shadedColor							= depthFogColorBlue * depth + shadedColor * (1.0f - depth);
	return shadedColor;
}

//////////////////////////////////////////////////////////////////////////

template<typename N>
A_CUDA_CPUGPU static ResultColor GetColorForRayResult_SimpleLitSurface(const Configuration& config, const RayMarchResult<N>& result)
{
	if (!result.Hit)
	{
		return GetColorForRayResult_SolidColor(config, result);
	}

	ResultColor shadedColor = GetColorForRayResult_Surfaces(config, result);

	// Shade
	shadedColor = shadedColor * Math::Clamp01(Math::Remap(0.0f, 1.0f, config.AMBIENT_LIGHT_AMOUNT, 1.0f, result.ShadowValue));

	// Depth Fog		
	const float depth					= Math::Clamp01(Math::Remap(-10.0f, 40.0f, 0.0f, 1.0f, result.Position.z + result.Position.w));
	const ResultColor depthFogColorBlue = ResultColor{20, 20, 35, 255};
	shadedColor							= depthFogColorBlue * depth + shadedColor * (1.0f - depth);
	return shadedColor;
}

//////////////////////////////////////////////////////////////////////////

private:

	template<typename N>
	A_CUDA_CPUGPU static ResultColor GetColorForRayResult_DistanceToSureface(const Configuration& config, const RayMarchResult<N>& result)
	{
		// Distance as color
		if (result.SignedDistance < config.RAY_HIT_EPSILON)
		{
			return ResultColor{
				255,
				255,
				255,
				255
			};
		}
		else
		{
			float colorValue = Math::Clamp(Math::Remap(120, 500, 0, 255, result.SignedDistance), 0.0f, 255.0f);
			colorValue *= (1.0f + 0.3f * std::cos(colorValue));
			colorValue *= std::exp(-0.001f * Math::Abs(result.SignedDistance));
			
			return ResultColor {
				static_cast<BufferType>(std::round(colorValue)),
				static_cast<BufferType>(std::round(colorValue)),
				static_cast<BufferType>(std::round(colorValue)),
				255
			};
		}
	}
	
	//////////////////////////////////////////////////////////////////////////

	template<typename N>
	A_CUDA_CPUGPU static ResultColor GetColorForRayResult_Shaded(const Configuration& config, const RayMarchResult<N>& result)
	{
		if (!result.Hit)
		{
			return GetColorForRayResult_SolidColor(config, result);
		}

		ResultColor shadedColor;
		float depth = 0.3f;

		ResultColor baseColor	= Math::GetCheckerboard(result.LocalPosition, config.CHECKERBOARD_SIZE);
		if (baseColor.Blue < 150)
		{
			// Light tile:
			ResultColor surfaceColor	= GetColorForRayResult_Surfaces(config, result);
			baseColor					= baseColor * 0.5f + surfaceColor * 0.5f;
		}
		shadedColor = baseColor * Math::Remap(0.0f, 1.0f, config.AMBIENT_LIGHT_AMOUNT, 1.0f, result.ShadowValue);
		depth		= Math::Clamp01(Math::Remap(-10.0f, 40.0f, 0.0f, 1.0f, result.Position.z + result.Position.w));

		// Depth Fog.		
		const ResultColor depthFogColorBlue = ResultColor{20, 20, 35, 255};

		shadedColor = depthFogColorBlue * depth /*+ depthFogColorOrange * depthW*/ + shadedColor * (1.0f - depth/* - depthW*/);

		return shadedColor;
	}
	
	//////////////////////////////////////////////////////////////////////////

	template<typename N>
	A_CUDA_CPUGPU static ResultColor GetColorForRayResult_ShadedNoFog(const Configuration& config, const RayMarchResult<N>& result)
	{
		if (!result.Hit)
		{
			return GetColorForRayResult_SolidColor(config, result);
		}

		ResultColor baseColor	= Math::GetCheckerboard(result.LocalPosition, config.CHECKERBOARD_SIZE);
		if (baseColor.Blue < 150)
		{
			// Light tile:
			ResultColor surfaceColor	= GetColorForRayResult_Surfaces(config, result);
			baseColor					= baseColor * 0.5f + surfaceColor * 0.5f;
		}
		const ResultColor shadedColor = baseColor * Math::Remap(0.0f, 1.0f, config.AMBIENT_LIGHT_AMOUNT, 1.0f, result.ShadowValue);
		return shadedColor;
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename N>
	A_CUDA_CPUGPU static ResultColor GetColorForRayResult_Normal(const Configuration& config, const RayMarchResult<N>& result)
	{
		return ResultColor{
			static_cast<BufferType>(255 * (result.Normal.x + 1.0f) / 2.0f),
			static_cast<BufferType>(255 * (result.Normal.y + 1.0f) / 2.0f),
			static_cast<BufferType>(255 * (result.Normal.z + 1.0f) / 2.0f),
			255
		};
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename N>
	A_CUDA_CPUGPU static ResultColor GetColorForRayResult_NormalConfigurable(const Configuration& config, const RayMarchResult<N>& result)
	{
		BufferType red		= 0;
		BufferType green	= 0;
		BufferType blue		= 0;
		BufferType alpha	= 255;
		
		if (config.DrawModeConfigurableColorByAxisIDs[0] != 0) red		= static_cast<BufferType>(255 * (result.Normal[config.DrawModeConfigurableColorByAxisIDs[0] - 1] + 1.0f) / 2.0f);
		if (config.DrawModeConfigurableColorByAxisIDs[1] != 0) green	= static_cast<BufferType>(255 * (result.Normal[config.DrawModeConfigurableColorByAxisIDs[1] - 1] + 1.0f) / 2.0f);
		if (config.DrawModeConfigurableColorByAxisIDs[2] != 0) blue		= static_cast<BufferType>(255 * (result.Normal[config.DrawModeConfigurableColorByAxisIDs[2] - 1] + 1.0f) / 2.0f);
		if (config.DrawModeConfigurableColorByAxisIDs[3] != 0) alpha	= static_cast<BufferType>(255 * (result.Normal[config.DrawModeConfigurableColorByAxisIDs[3] - 1] + 1.0f) / 2.0f);

		return ResultColor{red, green, blue, alpha};
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename N>
	A_CUDA_CPUGPU static ResultColor GetColorForRayResult_Steps(const Configuration& config, const RayMarchResult<N>& result)
	{
		float lerpFactor = result.Steps / static_cast<float>(config.MAX_STEPS);

		return ResultColor{
			static_cast<BufferType>(Math::UnclampedLerp(0.0f, 255.0f, lerpFactor)),
			static_cast<BufferType>(Math::UnclampedLerp(255.0f, 0.0f, lerpFactor)),
			0,
			255
		};
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename N>
	A_CUDA_CPUGPU static ResultColor GetColorForRayResult_TraversedPrimary(const Configuration& config, const RayMarchResult<N>& result)
	{
		float lerpFactor = Math::Remap(300.0f, 550.0f, 0.0f, 255.0f, result.TraversedPrimary);

		return ResultColor{
			static_cast<BufferType>(lerpFactor),
			static_cast<BufferType>(255.0f - lerpFactor),
			0,
			255
		};
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename N>
	A_CUDA_CPUGPU static ResultColor GetColorForRayResult_TraversedSecondary(const Configuration& config, const RayMarchResult<N>& result)
	{
		float lerpFactor = Math::Remap(50.0f, 200.0f, 0.0f, 255.0f, result.TraversedSecondary);

		return ResultColor{
			static_cast<BufferType>(lerpFactor),
			static_cast<BufferType>(255.0f - lerpFactor),
			0,
			255
		};
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename N>
	A_CUDA_CPUGPU static ResultColor GetColorForRayResult_CloseToGeo(const Configuration& config, const RayMarchResult<N>& result)
	{
		// Deprecated
		return ResultColor{
			255, // static_cast<BufferType>(result.CloseToGeo ? 255 : 0),
			0, // static_cast<BufferType>(result.CloseToGeo ? 0 : 255),
			255,
			255
		};
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename N>
	A_CUDA_CPUGPU static ResultColor GetColorForRayResult_Checker(const Configuration& config, const RayMarchResult<N>& result)
	{
		return Math::GetCheckerboard(result.LocalPosition, config.CHECKERBOARD_SIZE);
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename N>
	A_CUDA_CPUGPU static ResultColor GetColorForRayResult_ColoredChecker(const Configuration& config, const RayMarchResult<N>& result)
	{
		if (!result.Hit)
		{
			return GetColorForRayResult_SolidColor(config, result);
		}

		ResultColor baseColor	= Math::GetCheckerboard(result.LocalPosition, config.CHECKERBOARD_SIZE);
		if (baseColor.Blue < 150)
		{
			// Light tile:
			const ResultColor surfaceColor	= GetColorForRayResult_Surfaces(config, result);
			baseColor						= baseColor * 0.5f + surfaceColor * 0.5f;
		}
			
		return baseColor;
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename N>
	A_CUDA_CPUGPU static ResultColor GetColorForRayResult_SolidColor(const Configuration& config, const RayMarchResult<N>& result)
	{
		if (result.Hit)
		{
			return ResultColor{
				static_cast<BufferType>(config.DrawModeSolidColorHit.Value.x * 255),
				static_cast<BufferType>(config.DrawModeSolidColorHit.Value.y * 255),
				static_cast<BufferType>(config.DrawModeSolidColorHit.Value.z * 255),
				static_cast<BufferType>(config.DrawModeSolidColorHit.Value.w * 255)
			};
		}
		else
		{
			return ResultColor{
				static_cast<BufferType>(config.DrawModeSolidColorMiss.Value.x * 255),
				static_cast<BufferType>(config.DrawModeSolidColorMiss.Value.y * 255),
				static_cast<BufferType>(config.DrawModeSolidColorMiss.Value.z * 255),
				static_cast<BufferType>(config.DrawModeSolidColorMiss.Value.w * 255)
			};
		}
	}
	
	//////////////////////////////////////////////////////////////////////////

	template<typename N>
	A_CUDA_CPUGPU static ResultColor GetColorForRayResult_Position(const Configuration& config, const RayMarchResult<N>& result)
	{
		BufferType red	 = 0;
		BufferType green = 0;
		BufferType blue	 = 0;
		BufferType alpha = 255;

		const float percentage[4] = {
			Math::Clamp01(Math::InverseUnclampedLerp(config.DrawModePositionMin[0], config.DrawModePositionMax[0], result.Position.x)),
			Math::Clamp01(Math::InverseUnclampedLerp(config.DrawModePositionMin[1], config.DrawModePositionMax[1], result.Position.y)),
			Math::Clamp01(Math::InverseUnclampedLerp(config.DrawModePositionMin[2], config.DrawModePositionMax[2], result.Position.z)),
			Math::Clamp01(Math::InverseUnclampedLerp(config.DrawModePositionMin[3], config.DrawModePositionMax[3], result.Position.w))
		};
		
		if (config.DrawModeConfigurableColorByAxisIDs[0] != 0) red		= static_cast<BufferType>(255 * (percentage[config.DrawModeConfigurableColorByAxisIDs[0] - 1] + 1.0f) / 2.0f);
		if (config.DrawModeConfigurableColorByAxisIDs[1] != 0) green	= static_cast<BufferType>(255 * (percentage[config.DrawModeConfigurableColorByAxisIDs[1] - 1] + 1.0f) / 2.0f);
		if (config.DrawModeConfigurableColorByAxisIDs[2] != 0) blue		= static_cast<BufferType>(255 * (percentage[config.DrawModeConfigurableColorByAxisIDs[2] - 1] + 1.0f) / 2.0f);
		if (config.DrawModeConfigurableColorByAxisIDs[3] != 0) alpha	= static_cast<BufferType>(255 * (percentage[config.DrawModeConfigurableColorByAxisIDs[3] - 1] + 1.0f) / 2.0f);
		
		return ResultColor{red, green, blue, alpha};
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename N>
	A_CUDA_CPUGPU static ResultColor GetColorForRayResult_LocalPosition(const Configuration& config, const RayMarchResult<N>& result)
	{
		BufferType red	 = 0;
		BufferType green = 0;
		BufferType blue	 = 0;
		BufferType alpha = 255;

		const float percentage[4] = {
			Math::Clamp01(Math::InverseUnclampedLerp(config.DrawModePositionMin[0], config.DrawModePositionMax[0], result.LocalPosition.x)),
			Math::Clamp01(Math::InverseUnclampedLerp(config.DrawModePositionMin[1], config.DrawModePositionMax[1], result.LocalPosition.y)),
			Math::Clamp01(Math::InverseUnclampedLerp(config.DrawModePositionMin[2], config.DrawModePositionMax[2], result.LocalPosition.z)),
			Math::Clamp01(Math::InverseUnclampedLerp(config.DrawModePositionMin[3], config.DrawModePositionMax[3], result.LocalPosition.w))
		};
		
		if (config.DrawModeConfigurableColorByAxisIDs[0] != 0) red		= static_cast<BufferType>(255 * (percentage[config.DrawModeConfigurableColorByAxisIDs[0] - 1] + 1.0f) / 2.0f);
		if (config.DrawModeConfigurableColorByAxisIDs[1] != 0) green	= static_cast<BufferType>(255 * (percentage[config.DrawModeConfigurableColorByAxisIDs[1] - 1] + 1.0f) / 2.0f);
		if (config.DrawModeConfigurableColorByAxisIDs[2] != 0) blue		= static_cast<BufferType>(255 * (percentage[config.DrawModeConfigurableColorByAxisIDs[2] - 1] + 1.0f) / 2.0f);
		if (config.DrawModeConfigurableColorByAxisIDs[3] != 0) alpha	= static_cast<BufferType>(255 * (percentage[config.DrawModeConfigurableColorByAxisIDs[3] - 1] + 1.0f) / 2.0f);
		
		return ResultColor{red, green, blue, alpha};
	}


	//////////////////////////////////////////////////////////////////////////

	template<typename N>
	A_CUDA_CPUGPU static ResultColor GetColorForRayResult_W_Heat(const Configuration& config, const RayMarchResult<N>& result)
	{
		if (!result.Hit)
		{
			return GetColorForRayResult_SolidColor(config, result);
		}

		ResultColor shadedColor;

		ResultColor baseColor	= Math::GetCheckerboard(result.LocalPosition, config.CHECKERBOARD_SIZE / 2.0f);
		if (baseColor.Blue < 150)
		{			
			float heat					= Math::Clamp(Math::Remap(-20.f, 20.0f, 0.0f, 255.0f, result.Position.w), 0.0f, 255.0f);
			unsigned char heatBlue		= static_cast<unsigned char>(255.0f - heat);
			unsigned char heatRed		= static_cast<unsigned char>(heat);
			ResultColor surfaceColor	= ResultColor{heatRed, 0, heatBlue, 255};
			baseColor					= surfaceColor;
		}
		shadedColor = baseColor * Math::Remap(0.0f, 1.0f, config.AMBIENT_LIGHT_AMOUNT, 1.0f, result.ShadowValue);

		return shadedColor;
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename N>
	A_CUDA_CPUGPU static ResultColor GetColorForRayResult_Surfaces(const Configuration& config, const RayMarchResult<N>& result)
	{
		if (!result.Hit)
		{
			return GetColorForRayResult_SolidColor(config, result);
		}

		// TODO: Find out why N::length() is not a constexpr for the CUDA compiler, but is for the VS compiler. Then, soft code this. #CUDA_VS_VISUALSTUDIO
		constexpr int COMPONENT_COUNT = 4; 

		// Calculate an array like this:
		// [0, 0, 1, 1] -> both z & w coordinate of the given vector have the same, highest magnitude of the vector.

		float highestMagnitude = 0.0f;

		bool isComponentHighest[COMPONENT_COUNT];
		for (int i = 0; i < COMPONENT_COUNT; i++)
		{
			isComponentHighest[i] = false;
		}

		constexpr float NORMAL_INFLUENCE = 1.0f; // 5.0f

		// Look a bit inside the object to bettwer find the high component.
		N referencePosition = result.LocalPosition - result.LocalNormal * NORMAL_INFLUENCE;

		// Find highest magnitude component(s) of the given vector
		for (int i = 0; i < COMPONENT_COUNT; i++)
		{
			float magnitude = glm::abs(referencePosition[i]);

			float differenceToHighest = magnitude - highestMagnitude;

			// Update highest
			if (differenceToHighest > 0)
			{
				highestMagnitude		= magnitude;
			}

			// Update array of current / previous high components
			constexpr float THRESHHOLD = 0.005f;
			if (differenceToHighest > THRESHHOLD)
			{
				// Single highest
				isComponentHighest[i]	= true;

				for (int j = 0; j < i; j++)
				{
					// Set all previous highest to false.
					isComponentHighest[j] = false;
				}
			}
			else if (differenceToHighest < THRESHHOLD && differenceToHighest > -THRESHHOLD)
			{
				// Shared highest
				isComponentHighest[i]	= true;
			}
			else
			{
				// Not highest
				isComponentHighest[i]	= false;
			}
		}

		// Count number of high bits
		unsigned int amountHighComponents = 0;
		for (int i = 0; i < COMPONENT_COUNT; i++)
		{
			amountHighComponents += static_cast<unsigned int>(isComponentHighest[i]);
		}

		//////////////////////////////////////////////////////////////////////////

		if (amountHighComponents == 1)
		{
			// Case: Volume

			for (int i = 0; i < COMPONENT_COUNT; i++)
			{
				if (isComponentHighest[i])
				{
					bool isIPositive = referencePosition[i] >= 0.0f;
					return GetResultColorForHighComponentcomponentVolume(i, isIPositive, config.ActiveColorScheme);
				}
			}
		}
		else if (amountHighComponents == 2)
		{
			// Case: Surface
			// How did we hit a surface after subtracting the local normal, rather than a volume?
			return ResultColor{120, 120, 120, 255};
		}
		else
		{
			// Case: Edge, Corner

			// Return Black
			return Colors::DARK_GRAY;
		}
		
		return Colors::ERROR;
	}

	//////////////////////////////////////////////////////////////////////////

	A_CUDA_CPUGPU static ResultColor GetResultColorForHighComponentcomponentVolume(int componentID, bool isPositive, const Colors::ColorScheme& colorScheme)
	{
		// For each of the 8 volumes of a hypercube, a color is mapped.
		unsigned short colorIDInScheme = 2 * componentID + static_cast<unsigned int>(!isPositive);
		if (colorIDInScheme < Colors::PRIMITVE_COLOR_COUNT)
		{
			unsigned short colorIDInLookup = colorScheme.ColorIDs[colorIDInScheme];
			return Colors::GetColorByID(colorIDInLookup);
		}
		
		// Something went wrong.
		return Colors::ERROR;
	}
};