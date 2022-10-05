#pragma once
#include "glm/ext/vector_int2.hpp"

enum QuiltConfiguration
{
	_16_singleView,
	_512_singleView,
	_512_2x4,
	_512_4x8,
	_1k_4x8,
	_2k_1x1,
	_2k_4x8,
	_4k_5x9,
	_8k_5x9,

	Count
};

static const char* s_QuiltConfigurationsNames[(int) QuiltConfiguration::Count] = {
	"016x016_1x1",
	"512x512_1x1",
	"512x512_2x4",
	"512x512_4x8",
	"1024x1024_4x8",
	"2048x2048_1x1",
	"2048x2048_4x8",
	"4096x4096_5x9",
	"8192x8192_5x9",
};

//////////////////////////////////////////////////////////////////////////

struct QuiltConfigurationData
{
	QuiltConfigurationData() = default;

	void Initialize(const glm::ivec2& textureDimensions, const glm::ivec2& views, const glm::ivec2& tileSize) 
	{
		TextureDimensions		= textureDimensions; 
		Views					= views;
		TileSize				= tileSize;
		ViewDimensions			= {textureDimensions.x / views.x, textureDimensions.y /views.y};		// < this rounds down, as it is an int / int devision.
		ViewCount				= views.x * views.y;
		UsedTextureDimensions	= {ViewDimensions.x * views.x, ViewDimensions.y * views.y};

		assert(fmod(ViewDimensions.x, TileSize.x) == 0.0f && fmod(ViewDimensions.y, TileSize.y) == 0.0f);
	}
		
	glm::ivec2		TextureDimensions;
	glm::ivec2		UsedTextureDimensions;
	glm::ivec2		Views;
	glm::ivec2		ViewDimensions;
	unsigned int	ViewCount;
	glm::ivec2		TileSize;
};