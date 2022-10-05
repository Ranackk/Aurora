#include "stdafx.h"
#include "ExperimentTypes.h"
#include "Experiment.h"

Experiment* CreateExperiment(const ExperimentType type)
{
	switch (type)
	{
		case ExperimentType::BorderingPrimitves:
		{
			return new OppositePrimtiveExperiment();
		}
		break;
		default:
		{
			printf("Could not create experiment of type %i", static_cast<int>(type));
		}
		break;
	}

	return nullptr;
}
