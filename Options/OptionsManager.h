#pragma once

#include <Vendor/imgui/imgui.h>

#include "Macros.h"
#include "Options/Configuration.h"

class Application;

// Used to render the DEBUG UI
class OptionsManager
{
public:

	bool m_FirstTick = true;

	//////////////////////////////////////////////////////////////////////////
	// Rendering

	DEBUG_MUTABLE int			RENDER_RESOLUITION_X;
	DEBUG_MUTABLE int			RENDER_RESOLUITION_Y;	
	DEBUG_MUTABLE int			CAMERA_CAPTURE_WIDTH;	
	DEBUG_MUTABLE int			CAMERA_CAPTURE_HEIGHT;	

	DEBUG_MUTABLE int			MAX_STEPS;				
	DEBUG_MUTABLE float			MAX_DEPTH;
	DEBUG_MUTABLE float			RAY_HIT_EPSILON;
													
	DEBUG_MUTABLE float			CHECKERBOARD_SIZE;
	
	//////////////////////////////////////////////////////////////////////////
	// Scene

	DEBUG_MUTABLE float			MOVE_SPEED;

	//////////////////////////////////////////////////////////////////////////

	void DrawDebugWindows(Application& application, Configuration& configuration);
};

