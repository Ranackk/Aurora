#include "stdafx.h"
#include "ColorSchemes.h"

A_CUDA_CPUGPU ResultColor Colors::GetColorByID(unsigned int id)
{
	static ResultColor CUBE_COLORS[CUBE_COLOR_COUNT] = {RED, ORANGE, YELLOW, LIME, GREEN, CYAN, TEAL, AQUAMARINE, LIGHT_BLUE, NAVY, PURPLE};
	return CUBE_COLORS[id];
}

//////////////////////////////// //////////////////////////////////////////

unsigned int Colors::ColorScheme::GetOppositeID(unsigned int id)
{
	if (id % 2 == 0)
	{
		return id + 1;	
	}

	return id - 1;
}

//////////////////////////////// //////////////////////////////////////////

Colors::ColorScheme Colors::GenerateColorScheme(unsigned int seed)
{
	ColorScheme scheme;

	unsigned int matchedColors = 0;

	std::default_random_engine generator(seed);
	std::uniform_int_distribution<int> distribution(0, CUBE_COLOR_COUNT - 1);

	while (matchedColors < PRIMITVE_COLOR_COUNT)
	{
		// Find random index that is not yet part of colors array.
		int colorCandidate	= distribution(generator);
		bool isValid		= true;
		for (unsigned int i = 0; i < matchedColors; i++)
		{
			if (scheme.ColorIDs[i] == colorCandidate)
			{
				isValid = false;
				break;
			}
		}

		if (!isValid)
		{
			continue;
		}

		scheme.ColorIDs[matchedColors] = colorCandidate;
		matchedColors++;
	}

	return scheme;
}

//////////////////////////////// //////////////////////////////////////////

unsigned int Colors::ColorScheme::GetColorIDForButtonID(unsigned int seed, unsigned int id)
{
	// Based on a seed, does a mapping similar to
	// 0 -> 2
	// 1 -> 4
	// 2 -> 7
	// 3 -> 0
	// 4 -> 3
	// 5 -> 6
	// 6 -> 5
	// 7 -> 1
	// This is used in UI to make sure that there is no traceable connection between for instance first answer and positive-x-axis.

	std::default_random_engine generator(seed);
	std::uniform_int_distribution<int> distribution(0, PRIMITVE_COLOR_COUNT - 1);

	unsigned char matched = 0b00000000;
	unsigned int match = distribution(generator);
	for (unsigned int i = 0; i <= id; i++)
	{
		while (true)
		{
			match = distribution(generator);
			if (((1 << match) & matched) == 0)
			{
				matched |= (1 << match);
				break;
			}
		}
	}

	return match;
}