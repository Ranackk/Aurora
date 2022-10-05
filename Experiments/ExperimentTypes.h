#pragma once
#include "Options/Configuration.h"

struct Experiment;

enum class ExperimentType
{
	BorderingPrimitves	= 0,
	RotateOpposite		= 1, // < Not implemented
	JudgeHypervolume	= 2, // < Not implemented

	Count				= 3
};

Experiment* CreateExperiment(const ExperimentType type);

enum class ExperimentStatus
{
	InExperiment			= 0,
	ExperimentDone			= 1
};

enum class Gender
{
	Male,
	Female,
	Other,

	Count = 3
};

static const char* s_GenderNames[(int)Gender::Count] = {
	"Male",
	"Female",
	"Other"
};


enum class AcademicBackground
{
	Art,
	Design,
	Science,
	Other,

	Count = 4
};

static const char* s_AcademicBackgroundNames[(int)AcademicBackground::Count] = {
	"Art",
	"Design",
	"Science",
	"Other"
};

//////////////////////////////////////////////////////////////////////////

enum class DerivationFlags : unsigned short
{
	Base					= 0 << 0,
	AutostereoscopicView	= 1 << 0,
	Shading					= 1 << 1,
	Texture					= 1 << 2,

	Count					= 3
};

static const char* s_DerivationFlagNames[(int)DerivationFlags::Count] = {
	"3D",
	"Shading",
	"Texture"
};
	
static inline bool CheckDerivationFlag(const unsigned short derivation, const DerivationFlags flag)
{
	return (derivation & static_cast<unsigned short>(flag)) != 0;
};

static inline Configuration::DrawMode GetDrawMode(const unsigned short derivation)
{
	const bool useShading = CheckDerivationFlag(derivation, DerivationFlags::Shading);
	const bool useTexture	= CheckDerivationFlag(derivation, DerivationFlags::Texture);

	if (useShading)
	{
		return useTexture ? Configuration::DrawMode::Shaded : Configuration::DrawMode::SimpleLitSurface;
	}

	return useTexture ? Configuration::DrawMode::ColoredChecker : Configuration::DrawMode::Surfaces;
};