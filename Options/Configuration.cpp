#include "stdafx.h"

#include "Configuration.h"

const char* Configuration::s_DrawModeNames[(int)DrawMode::Count] = {
	"Shaded",
	"ShadedNoFog",
	"SimpleLit",
	"SimpleLitSurface",
	"Normals",
	"NormalsConfigurable",
	"Steps",
	"TraversedPrimary",
	"TraversedSecondary",
	"SolidColor",
	"Position",
	"LocalPosition",
	"Distance",
	"Transparent",
	"CloseToGeo",
	"Checker",
	"ColoredChecker",
	"Surfaces",
	"W_Heat"
};

const char* Configuration::s_AxisNames[5] = {
	"/",
	"x",
	"y",
	"z",
	"w"
};