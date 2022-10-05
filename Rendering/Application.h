#pragma once

#include <vector>
#include <array>
#include <thread>
#include <future>
#include <chrono>
#include <math.h>
#include <iomanip>
#include <corecrt_math_defines.h>

#include "GraphicsIncludes.h"

#include "Vendor/bitmap_image.hpp"

#include "MathLib/MathLib.h"

#include "Experiments/StudyManager.h"
#include "Marching/MarchingTypes.h"
#include "Options/OptionsManager.h"
#include "Options/Configuration.h"
#include "Rendering/CUDATypes.h"
#include "Rendering/CUDAInterface.h"
#include "Rendering/Camera.h"
#include "Rendering/Light.h"
#include "Rendering/QuiltTypes.h"
#include "Scenes/Scene.h"


#define GL_CHECK_ERROR() TestOpenGlError(__FILE__, __LINE__);

using DimensionVector = glm::vec4;
using DimensionMatrix = glm::mat4x4;

//////////////////////////////////////////////////////////////////////////

class Application
{
public:
	static Application* s_Instance;

private:

	constexpr static unsigned int DEV_INDEX						= 0;	// < Which holo display to use, if multiple are connected
	constexpr static unsigned int RESULT_COLOR_COMPONENT_COUNT	= 4;	// < r, g, b, a

	//////////////////////////////////////////////////////////////////////////
	// Command Line

	bool m_UseLookingGlassAsDisplay = true;

	//////////////////////////////////////////////////////////////////////////

	// Rendering [Move to "Renderer"?]
	const char* GLSL_VERSION_IMGUI		= "#version 130"; // + "#version 330 core"?

	GLFWwindow* m_Window;
	glm::ivec2	m_WindowSize;
	glm::ivec2	m_WindowPosition = glm::ivec2(1500, 1000);

	//////////////////////////////////////////////////////////////////////////
	// HoloPlay
	
	GLuint m_HPBlitShaderHandle;
	GLuint m_HPLightFieldShaderHandle;

	public:

	QuiltConfiguration	ActiveQuiltConfiguration;
	bool				DrawQuiltInsteadOfLightfield = false;

	private:
	QuiltConfigurationData	m_QuiltConfigData;

	//////////////////////////////////////////////////////////////////////////
	// Screen Quad

	GLuint m_ScreenQuadVAOHandle;
	GLuint m_ScreenQuadVBOHandle;
	GLuint m_ScreenQuadEBOHandle;
	GLuint m_ScreenQuadShaderHandle;
	
	//////////////////////////////////////////////////////////////////////////
	// Threading
	// mt	= shared via c++ threads
	// mh	= on host
	// md	= on device
	// mcm	= CUDA managed
	// mcmt = CUDA managed + shared via c++ threads	

	static constexpr unsigned int BUFFER_COUNT = 2;

	unsigned int										m_RenderingBufferIDMainThread			= 0;
	unsigned int							 			m_RenderingBufferIDRaymarchingThread	= 1;

	std::array<RenderingBuffer, BUFFER_COUNT>			m_RenderingBuffers = {};
	cudaSurfaceObject_t									md_RenderingSurfaceObject;

	RenderPixelBufferDataCUDA*							mcm_RenderBufferData;
	RenderSceneDataCUDA*								mcm_RenderSceneData;
	Camera<DimensionVector>*							mcm_Camera;
	Light<DimensionVector>*								mcm_Light;
	Configuration*										mh_Configuration;
	Configuration*										md_Configuration;

	cudaArray_t											m_VoxelGridBuffer;
	RenderVoxelBufferDataCUDA*							mcm_VoxelGridData;

	std::thread											m_RaymarchThread;
	std::promise<void>									m_RaymarchThreadExitSignal;

	std::atomic<bool>									mt_StartRender						= {false};
	std::atomic<bool>									mt_IsCopingFromABuffer					= {false};
	std::array<std::atomic<bool>, BUFFER_COUNT>			mt_IsWritingIntoBuffer					= {}; // false;

	//////////////////////////////////////////////////////////////////////////

	// Scene
	SceneHyperPlayground*								mcm_Scene;

	// Imgui
	glm::vec4											m_ClearColorImgui = glm::vec4(0.45f, 0.55f, 0.60f, 1.00f);
		 	
	std::time_t											m_ApplicationStartupTime		= std::time(nullptr);
	std::chrono::system_clock::time_point				m_ApplicationStartupTimePoint	= std::chrono::system_clock::now();
	std::chrono::microseconds							m_LastRayMarchingTimeMyS		= std::chrono::microseconds(0);
	std::chrono::microseconds							m_LastMainLoopTimeMyS			= std::chrono::microseconds(0);

	//////////////////////////////////////////////////////////////////////////
	// Manager
	
	OptionsManager*										m_OptionsManager;
	StudyManager*										m_StudyManager;

public:

	Application() = default;
	~Application() = default;

	auto						GetScene() { return mcm_Scene; }
	int							Run(const bool useLookingGlassAsDisplay);

	// Time
	std::time_t					GetApplicationStartupTime() const	{ return m_ApplicationStartupTime; }
	std::chrono::microseconds	GetTimeSinceStartupMyS() const		{ return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - m_ApplicationStartupTimePoint); }
	std::chrono::microseconds	LastRayMarchingTimeMyS() const		{ return m_LastRayMarchingTimeMyS; }
	std::chrono::microseconds	LastMainLoopTimeMyS() const			{ return m_LastMainLoopTimeMyS; }
	static Application*			Instance()							{ return s_Instance; }

	// Getters
	Configuration*				GetHostConfiguration()				{ return mh_Configuration; }
	glm::ivec2					GetWindowPosition() const			{ return m_WindowPosition; }
	glm::ivec2					GetWindowSize() const				{ return m_WindowSize; }
	
	//////////////////////////////////////////////////////////////////////////

	void						UpdateWindowPosition(glm::ivec2 newPosition) { m_WindowPosition = newPosition; }

	//////////////////////////////////////////////////////////////////////////
	// Debug

	DimensionVector		m_LastCameraPosition;
	DimensionVector		m_LastCameraRight;	
	DimensionVector		m_LastCameraUp;	
	DimensionVector		m_LastCameraForward;
	DimensionVector		m_LastCameraOver;

	ProjectionMethod	m_DesiredCameraProjectionMethodMain			= ProjectionMethod::Perspectve;
	ProjectionMethod	m_DesiredCameraProjectionMethodSecondary		= ProjectionMethod::Perspectve;
	
	float				m_DesiredCameraPaneScaleZ = 1.0f;
	float				m_DesiredCameraPaneScaleW = 1.0f;

	DimensionVector		m_DesiredLightPosition;
	float				m_DesiredLightRadius;

	//////////////////////////////////////////////////////////////////////////
	// Camera Control

	const float CAMERA_ANGLE_ZW_DEFAULT = glm::radians(-90.0f);

	float m_CameraAngleZW = CAMERA_ANGLE_ZW_DEFAULT;
	float m_CameraAngleYZ = 0.0f;
	float m_CameraAngleXY = 0.0f;

private:

	//////////////////////////////////////////////////////////////////////////
	// Core

	A_CUDA_CPU void MainLoop();
	A_CUDA_CPU void ProcessInput();
	A_CUDA_CPU void ProcessCameraInput();
	void ApplyCameraInput();
	void ApplyLightInput();
	
	//////////////////////////////////////////////////////////////////////////
	// Rendering

	void ReInitRendering(QuiltConfiguration option);

	void InitRendering();
	void CleanupRendering();

	void MarchSample(int id, float percentageX, float percentageY, float percentageView = 0.5f); // For debugging purposes
	static void RenderingThread(Application* application, std::future<void> exitFuture);

	//////////////////////////////////////////////////////////////////////////
	// Application

	void InitManagers();
	void CleanupManagers();

	// Scene
	void InitScene();
	void CleanupScene();

	//////////////////////////////////////////////////////////////////////////
	// Plugins

	// Looking Glass Display
	void InitBaseShaders();
	void InitForNormalDisplay();
	bool InitLookingGlass();
	void CleanupLookingGlass();
	void SetupLookingGlass();
	void SetupForNormalDisplay();
	void ConfigureQuilt(QuiltConfiguration option);
	void PassQuiltSettingsToShader() const;

	// Rendering with OpenGL
	bool InitOpenGL();
	void CleanupOpenGL();

	void TestOpenGlError(const char* file, unsigned int line) const;
	
	void DrawScreenQuad() const;

	// Imgui - taken from https://github.com/ocornut/imgui, which was also used as a reference
	void InitImgui();
	void CleanupImgui();

	void BeginImgui();
	void EndImgui();

	void UpdateManagers();

};

