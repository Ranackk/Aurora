#pragma once
#include <random>
#include <Rendering/CUDATypes.h>

#include "Marching/MarchingTypes.h"

namespace Colors
{
	constexpr static ResultColor RED		= ResultColor{255, 0, 0, 255};	
	constexpr static ResultColor ORANGE		= ResultColor{255, 69, 0, 255};	

	constexpr static ResultColor YELLOW		= ResultColor{255, 255, 0, 255};
	constexpr static ResultColor OLIVE		= ResultColor{100, 80, 0, 255};	
	constexpr static ResultColor LIME		= ResultColor{40, 205, 40, 255};
	constexpr static ResultColor GREEN		= ResultColor{0, 100, 0, 255};
	
	constexpr static ResultColor CYAN		= ResultColor{0, 255, 255, 255};	
	constexpr static ResultColor TEAL		= ResultColor{0, 128, 128, 255};	
	constexpr static ResultColor AQUAMARINE = ResultColor{102, 205, 170, 255};	

	constexpr static ResultColor LIGHT_BLUE	= ResultColor{100, 100, 255, 255};	
	constexpr static ResultColor NAVY		= ResultColor{0, 0, 128, 255};	
	
	constexpr static ResultColor PURPLE		= ResultColor{254, 0, 254, 255};	
	
	constexpr static ResultColor ERROR		= ResultColor{255, 0, 255, 255};	
	constexpr static ResultColor WHITE		= ResultColor{255, 255, 255, 255};	
	constexpr static ResultColor GRAY		= ResultColor{128, 128, 128, 255};	
	constexpr static ResultColor DARK_GRAY	= ResultColor{30, 30, 30, 255};	
	constexpr static ResultColor BLACK		= ResultColor{0, 0, 0, 255};	

	constexpr unsigned short CUBE_COLOR_COUNT		= 11;
	constexpr unsigned short PRIMITVE_COLOR_COUNT	= 8;

	A_CUDA_CPUGPU ResultColor GetColorByID(unsigned int id);

	static const char* s_CubeColorNames[CUBE_COLOR_COUNT] = {
		"Red",
		"Orange",
		"Yellow",
		"Lime",
		"Green",
		"Cyan",
		"Teal",
		"Aquamarine",
		"Light Blue",
		"Navy",
		"Purple",
	};
	
	//////////////////////////////////////////////////////////////////////////

	struct ColorScheme
	{
		unsigned short ColorIDs[PRIMITVE_COLOR_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0};
		
		static unsigned int GetOppositeID(unsigned int id);
		static unsigned int GetColorIDForButtonID(unsigned int seed, unsigned int id);
	};
	//////////////////////////////////////////////////////////////////////////

	ColorScheme GenerateColorScheme(unsigned int seed);

	//////////////////////////////////////////////////////////////////////////


}
