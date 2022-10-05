#pragma once

#include "Macros.h"
#include <Vendor/imgui/imgui.h>
#include "Experiments/ColorSchemes.h"

struct Configuration
{
	//////////////////////////////////////////////////////////////////////////
	// Renderer

	RELEASE_CONST int	MAX_STEPS				= 1000; 		
	RELEASE_CONST float	MAX_DEPTH				= 4096.0f;
	RELEASE_CONST float	RAY_HIT_EPSILON			= 0.20f;
	RELEASE_CONST float	MIN_STEP_SIZE			= 0.15f;
	
	RELEASE_CONST float	SHADOW_START_OFFSET		= 0.05f;
	RELEASE_CONST float	SHADOW_RAY_HIT_EPSILON	= 0.2f;
	RELEASE_CONST int	MAX_STEPS_SHADOW		= 800;
	RELEASE_CONST float	SHADOW_PENUMBRA			= 2.0f;
											
	RELEASE_CONST float	CHECKERBOARD_SIZE		= 10.0f;

	RELEASE_CONST float	AMBIENT_LIGHT_AMOUNT	= 0.4f;

	//////////////////////////////////////////////////////////////////////////
	// Visualization

	enum class DrawMode : unsigned int
	{
		Shaded				= 0,
		ShadedNoFog			= 1,
		SimpleLit			= 2,
		SimpleLitSurface	= 3,
		NormalFast			= 4,
		NormalConfigurable	= 5,
		Steps				= 6,
		TraversedPrimary	= 7,
		TraversedSecondary	= 8,
		SolidColor			= 9,
		Position			= 10,
		LocalPosition		= 11,
		DistanceToSurface	= 12,
		Transparent			= 13,
		CloseToGeo			= 14,
		Checker				= 15,
		ColoredChecker		= 16,
		Surfaces			= 17,
		W_Heat				= 18,

		Count				= 19
	};

	static const char* s_DrawModeNames[(int) DrawMode::Count];
	static const char* s_AxisNames[5];

	DrawMode	DrawModeHit						= DrawMode::Shaded;
	DrawMode	DrawModeMiss					= DrawMode::Shaded;

	ImColor		DrawModeSolidColorHit			= ImColor(255, 0, 255, 255);
	ImColor		DrawModeSolidColorMiss			= ImColor(50, 50, 50, 255);

	int			DrawModeConfigurableColorByAxisIDs[4]	= {1, 2, 3, 0};
	
	float		DrawModePositionMin[4]			= {-20, -20, -20, -20};		// use this with the axis color mapping above.
	float		DrawModePositionMax[4]			= {20, 20, 20, 20};

	//////////////////////////////////////////////////////////////////////////

	const float STUDY_PERSPECTIVE[6]			= {0.785f, 0.660f, 4.150f - 6.28f, 6.24f - 6.28f, 6.5f - 6.28f, 0.0f}; 
	
	float		SceneSliderRotations[6]			= {0.785f, 0.660f, 4.150f - 6.28f, 6.24f - 6.28f, 6.5f - 6.28f, 0.0f}; 
	float		SceneSliderPositions[4]			= {};
	bool		SceneAnimateRotations[6]		= {};

	float		SceneSpeed						= 0.6f;

	//////////////////////////////////////////////////////////////////////////

	// Experiments

	Colors::ColorScheme ActiveColorScheme			= Colors::GenerateColorScheme(42);

};