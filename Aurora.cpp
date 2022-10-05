#include "stdafx.h"

// This needs to happen before anything else!
#define GLM_FORCE_CUDA

#include "Rendering/Application.h"

//////////////////////////////////////////////////////////////////////////

extern int kernelTest();

int main(int argc, char *argv[])
{
	//int ret = kernelTest();

	bool useLookingGlassAsDisplay = true;
	if (argc != 0)
	{
		for (int i = 0; i < argc; ++i) 
			std::cout << argv[i] << "\n"; 

		for (int i = 0; i < argc; i++)
		{
			if (strcmp(argv[i], "-window") == 0 || strcmp(argv[i], "-windowed") == 0)
			{
				useLookingGlassAsDisplay = false;
			}	
		}
	}
	Application application = Application();
	return application.Run(useLookingGlassAsDisplay);
}
