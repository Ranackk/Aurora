#include "stdafx.h"

#include <string>  
#include <iostream>
#include <iosfwd>
#include <filesystem>
#include <sstream> 
#include <stdio.h>
#include <thread>
#include <glad/glad.h>
#include <HoloPlayCore.h>
#include <HoloPlayShaders.h>
#include <string.h>
#include <time.h>

#include "Rendering/Application.h"
#include "Rendering/ShaderUtility.h"
#include "Marching/MarchingFunctions.h"

#include "Vendor/imgui/imgui.h"
#include "Vendor/imgui/imgui_impl_glfw.h"
#include "Vendor/imgui/imgui_impl_opengl3.h"
#include "Vendor/bitmap_image.hpp"

#include "Options/OptionsManager.h"

#include <cuda_gl_interop.h>
#include <corecrt.h>
#include <direct.h>

Application* Application::s_Instance;

// CUDA Functions defined in Application.cu
extern void CUDA_RenderImage(RenderPixelBufferDataCUDA* bufferData, RenderSceneDataCUDA* sceneData, Configuration* config, Camera<glm::vec4>* camera, Light<glm::vec4>* light);
extern void CUDA_PrepareRenderImage(RenderingBuffer& raymarchingBuffer, cudaSurfaceObject_t& outSurfaceObject);
extern void CUDA_FinishRenderImage(RenderingBuffer& raymarchingBuffer);

//////////////////////////////////////////////////////////////////////////
// Global variables used for controls
// Note that these are only used in this cpp, so for now it is okay that they are defined here.

bool g_KeyA = false;
bool g_KeyD = false;
bool g_KeyW = false;
bool g_KeyS = false;
bool g_KeyQ = false;
bool g_KeyE = false;
bool g_KeyY = false;
bool g_KeyX = false;
bool g_KeyR = false;
bool g_KeyO = false;

bool g_DebugMarchRay		= false;
bool g_DebugReMarchRay		= false;
bool g_DebugMarchRayMulti	= false;
glm::vec2 g_DebugMarchRayPercentage;

//////////////////////////////////////////////////////////////////////////

int Application::Run(bool useLookingGlassAsDisplay)
{
	s_Instance = this;

	m_UseLookingGlassAsDisplay = useLookingGlassAsDisplay;

	std::cout << "Hello World!\n";

	//////////////////////////////////////////////////////////////////////////
	// 1) Initialize

	bool lookingGlassSuccessfullyInitialized = false;
	lookingGlassSuccessfullyInitialized = InitLookingGlass(); 
	if (!useLookingGlassAsDisplay || !lookingGlassSuccessfullyInitialized)
	{
		InitForNormalDisplay();
		DrawQuiltInsteadOfLightfield = !lookingGlassSuccessfullyInitialized;
	}

	if (!InitOpenGL()) return -1;
	InitBaseShaders();

	if (lookingGlassSuccessfullyInitialized)
	{
		SetupLookingGlass();
	}
	else
	{
		SetupForNormalDisplay();
	}

	InitImgui();
	InitScene();
	InitRendering();

	InitManagers();

	//////////////////////////////////////////////////////////////////////////
	// 2) Main Loop
	
	MainLoop();

	//////////////////////////////////////////////////////////////////////////
	// 3) Cleanup
	
	CleanupManagers(); 

	CleanupRendering();
	CleanupScene();
	CleanupImgui();
	CleanupOpenGL();
	if (lookingGlassSuccessfullyInitialized)
	{
		CleanupLookingGlass();
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////

void Application::InitManagers()
{
	m_OptionsManager	= new OptionsManager();
	m_StudyManager		= new StudyManager();

	mh_Configuration = new Configuration();
	cudaMalloc(reinterpret_cast<void**> (&md_Configuration), sizeof(Configuration));
}

//////////////////////////////////////////////////////////////////////////

void Application::CleanupManagers()
{
	cudaFree(mh_Configuration);
	delete mh_Configuration;

	delete m_StudyManager;
	delete m_OptionsManager;
}

//////////////////////////////////////////////////////////////////////////+
// Open GL
//////////////////////////////////////////////////////////////////////////

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);
void WindowPositionCallback(GLFWwindow* window, int posX, int posY);

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

bool Application::InitOpenGL()
{
	//////////////////////////////////////////////////////////////////////////
	// Create Window

	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, true);

	m_Window = glfwCreateWindow(m_WindowSize.x, m_WindowSize.y, "Aurora", nullptr, nullptr);
	if (m_Window == nullptr)
	{

		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}

	if (m_UseLookingGlassAsDisplay)
	{
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	
		glfwWindowHint(GLFW_CENTER_CURSOR, false);
		glfwWindowHint(GLFW_DECORATED, false);
	}
	glfwSetWindowPos(m_Window, m_WindowPosition.x, m_WindowPosition.y);	

	glfwMakeContextCurrent(m_Window);
	glfwSetWindowPosCallback(m_Window, &WindowPositionCallback);
	glfwSetFramebufferSizeCallback(m_Window, &FrameBufferSizeCallback);
	glfwSwapInterval(1);	// < Enable Vsync

	//////////////////////////////////////////////////////////////////////////
	// Load up GLAD

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return false;
	}

	//////////////////////////////////////////////////////////////////////////

	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(MessageCallback, 0);

	GL_CHECK_ERROR();

	//////////////////////////////////////////////////////////////////////////

	cudaDeviceProp deviceProp;
	int deviceId;

	memset(&deviceProp, 0, sizeof(cudaDeviceProp));
	deviceProp.major = 3;
	deviceProp.minor = 0;
	CUDA_CHECK_ERROR(cudaChooseDevice(&deviceId, &deviceProp) );

	//////////////////////////////////////////////////////////////////////////
	// Shaders

	m_ScreenQuadShaderHandle = ShaderUtility::LoadUpShader("Shaders/SimpleTexture.vert", "Shaders/SimpleTexture.frag");

	//////////////////////////////////////////////////////////////////////////
	// Texture Buffers

	const float vertices[] = {
		// positions         // uvs
		1.0f,   1.0f, 0.0f,  1.0f, 1.0f,	 // top right
		1.0f,  -1.0f, 0.0f,  1.0f, 0.0f,	 // bottom right
		-1.0f, -1.0f, 0.0f,  0.0f, 0.0f,	 // bottom left
		-1.0f,  1.0f, 0.0f,  0.0f, 1.0f		 // top left 
	};

	const unsigned int indices[] = {
		0, 1, 3,							// first triangle
		1, 2, 3								// second triangle
	};

	GL_CHECK_ERROR();

	// Create VertexArrayObject
	glGenVertexArrays(1, &m_ScreenQuadVAOHandle);
	glBindVertexArray(m_ScreenQuadVAOHandle);

	// Create VertexBuffeeObject
	glGenBuffers(1, &m_ScreenQuadVBOHandle);
	glBindBuffer(GL_ARRAY_BUFFER, m_ScreenQuadVBOHandle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Create ElementBufferObject
	glGenBuffers(1, &m_ScreenQuadEBOHandle);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ScreenQuadEBOHandle);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	
	GL_CHECK_ERROR();

	// Specify attributes
	// Attribute 1: Position (size 3)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Attribute 2: UV (size 2)
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	
	GL_CHECK_ERROR();

	return true;
}

//////////////////////////////////////////////////////////////////////////

void Application::CleanupOpenGL()
{
	glDeleteVertexArrays(1, &m_ScreenQuadVAOHandle);
	glDeleteBuffers(1, &m_ScreenQuadVBOHandle);
	glDeleteBuffers(1, &m_ScreenQuadEBOHandle);

	glfwTerminate();
}

//////////////////////////////////////////////////////////////////////////

void WindowPositionCallback(GLFWwindow* window, int posX, int posY)
{
	Application::Instance()->UpdateWindowPosition(glm::vec2(posX, posY));
}

//////////////////////////////////////////////////////////////////////////

void Application::InitRendering()
{ 
	GL_CHECK_ERROR();

	//////////////////////////////////////////////////////////////////////////

	// Initialize Render Data
	CUDA_CHECK_ERROR(cudaMallocManaged(reinterpret_cast<void**>(&mcm_RenderBufferData), sizeof(decltype(mcm_RenderBufferData))));
	CUDA_CHECK_ERROR(cudaMallocManaged(reinterpret_cast<void**>(&mcm_RenderSceneData), sizeof(decltype(mcm_RenderSceneData))));

	//////////////////////////////////////////////////////////////////////////
	
	// Textures
	for (unsigned int i = 0; i < BUFFER_COUNT; i++)
	{
		RenderingBuffer& buffer = m_RenderingBuffers[i];
		
		glGenTextures(1, &buffer.TextureHandle);
		glBindTexture(GL_TEXTURE_2D, buffer.TextureHandle);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// Upload an empty texture into the correct space to already allocate the texture.. Last parameter = 0.	
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_QuiltConfigData.UsedTextureDimensions.x, m_QuiltConfigData.UsedTextureDimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glBindTexture(GL_TEXTURE_2D, 0);
		GL_CHECK_ERROR();

		CUDA_CHECK_ERROR(cudaGraphicsGLRegisterImage(&buffer.d_CUDAGraphicsResource, buffer.TextureHandle, GL_TEXTURE_2D, cudaGraphicsRegisterFlagsSurfaceLoadStore /*cudaGraphicsMapFlagsWriteDiscard*/));
	}

	//////////////////////////////////////////////////////////////////////////

	// Light
	
	CUDA_CHECK_ERROR(cudaMallocManaged(reinterpret_cast<void**>(&mcm_Light), sizeof(decltype(mcm_Light))));

	m_DesiredLightPosition	= {107, 75, -23, 30};
	m_DesiredLightRadius	= 100.0f;

	mcm_Light->Initialize(m_DesiredLightPosition, m_DesiredLightRadius);

	//////////////////////////////////////////////////////////////////////////

	// Camera
	
	CUDA_CHECK_ERROR(cudaMallocManaged(reinterpret_cast<void**>(&mcm_Camera), sizeof(decltype(mcm_Camera))));

	const float aspectRatio			= 2560.0f / 1600.0f;
	const DimensionVector right		= {1, 0, 0, 0};
	const DimensionVector up		= {0, 1, 0, 0};
	const DimensionVector forward	= {0, 0, 1, 0};
	const DimensionVector over		= {0, 0, 0, 1};
	
	const glm::mat4x4 rotMat = Math::RotXW(glm::radians(0.f)); // No rotation

	constexpr float CAMERA_DISTANCE = 400;

	constexpr float fovVert				= glm::radians(14.0f);
	constexpr float viewConeHorizontal	= glm::radians(35.0f);
	const DimensionVector camPos		= {0, 0 /* 275*/, -CAMERA_DISTANCE, -128};
	const float viewPaneDistance		= CAMERA_DISTANCE;

	mcm_Camera->Initialize(camPos, fovVert, viewConeHorizontal, aspectRatio, rotMat * forward, rotMat * right, rotMat * up, rotMat * over, viewPaneDistance, m_DesiredCameraProjectionMethodMain, m_DesiredCameraProjectionMethodSecondary);
}

//////////////////////////////////////////////////////////////////////////

void Application::ReInitRendering(QuiltConfiguration option)
{	
	// 1) Configure new quilt
	ConfigureQuilt(option);
	PassQuiltSettingsToShader(); 
	GL_CHECK_ERROR();

	//////////////////////////////////////////////////////////////////////////

	// 2) Update resources
	// 2a) Textures
	for (unsigned int i = 0; i < BUFFER_COUNT; i++)
	{
		RenderingBuffer& buffer = m_RenderingBuffers[i];
		
		if (buffer.IsCurrentlyMapped)
		{
			CUDA_CHECK_ERROR(cudaGraphicsUnmapResources(1, &m_RenderingBuffers[i].d_CUDAGraphicsResource));
			buffer.IsCurrentlyMapped = false;
		}

		glDeleteTextures(1, &buffer.TextureHandle);

		glGenTextures(1, &buffer.TextureHandle);
		glBindTexture(GL_TEXTURE_2D, buffer.TextureHandle);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		
		// Upload an empty texture into the correct space to already allocate the texture.. Last parameter = 0.	
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_QuiltConfigData.UsedTextureDimensions.x, m_QuiltConfigData.UsedTextureDimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glBindTexture(GL_TEXTURE_2D, 0);
		GL_CHECK_ERROR();

		CUDA_CHECK_ERROR(cudaGraphicsGLRegisterImage(&buffer.d_CUDAGraphicsResource, buffer.TextureHandle, GL_TEXTURE_2D, cudaGraphicsRegisterFlagsSurfaceLoadStore /*cudaGraphicsMapFlagsWriteDiscard*/));
	}
}

//////////////////////////////////////////////////////////////////////////

void Application::CleanupRendering()
{
	// Camera
	CUDA_CHECK_ERROR(cudaFree(mcm_Camera));

	// Light
	CUDA_CHECK_ERROR(cudaFree(mcm_Light));

	// Cleanup Render Data
	CUDA_CHECK_ERROR(cudaFree(mcm_RenderSceneData));
	CUDA_CHECK_ERROR(cudaFree(mcm_RenderBufferData));
}

//////////////////////////////////////////////////////////////////////////

void Application::MarchSample(int id, float percentageX, float percentageY, float percentageView)
{
	DimensionMatrix biRaySpaceToWorldSpace;
	const Math::BiRay<DimensionVector> biRay = mcm_Camera->GetBiray(percentageView, percentageX, percentageY, biRaySpaceToWorldSpace);
	
	// March TestRay
	const auto result						= RayMarchFunctions::MarchSingleBiRay<DimensionVector>(biRay, biRaySpaceToWorldSpace, mcm_RenderSceneData, 
																								   mh_Configuration->MIN_STEP_SIZE, mh_Configuration->MAX_DEPTH, 
																								   mh_Configuration->MAX_STEPS, mh_Configuration->RAY_HIT_EPSILON);

	const DimensionMatrix worldSpaceToBiRaySpace	= glm::inverse(biRaySpaceToWorldSpace);
	
	const DimensionVector hitPositionBS			= worldSpaceToBiRaySpace * result.Position;
	const DimensionVector closestPositionBS		= worldSpaceToBiRaySpace * result.ClosestPosition;
	const DimensionVector originPositionBS		= worldSpaceToBiRaySpace * biRay.Origin;

	// Sample Grid
	constexpr int GRID_SIZE	= 512;
	const float STEP_SIZE	= result.Hit ? 0.06f : 0.1f;
	
	const float START_MAIN	= result.Hit ? hitPositionBS.z - originPositionBS.z - 10 : closestPositionBS.z - originPositionBS.z - 30;
	const float START_SEC	= result.Hit ? hitPositionBS.w - originPositionBS.w - 10 : closestPositionBS.w - originPositionBS.w - 30;
	
	constexpr float DRAW_MAX_DISTANCE = 15.0f;

	bitmap_image imageDistance(GRID_SIZE, GRID_SIZE);
	bitmap_image imageDirection(GRID_SIZE, GRID_SIZE);
	bitmap_image imagePath(GRID_SIZE, GRID_SIZE);
	bitmap_image imageCombined(GRID_SIZE, GRID_SIZE * 2);
	imageDistance.set_all_channels(255, 0, 255);
	imageDirection.set_all_channels(255, 0, 255);
	imageCombined.set_all_channels(255, 0, 255);
	imagePath.set_all_channels(200, 200, 20);
	
	image_drawer drawerDirection(imageDirection);
	image_drawer drawerCombined(imageCombined);
	
	drawerDirection.pen_width(1);
	drawerCombined.pen_width(1);

	for (int iM = 0; iM < GRID_SIZE; iM ++)
	{
		for (int iS = 0; iS < GRID_SIZE; iS ++)
		{
			const float traversedMain		= START_MAIN + iM * STEP_SIZE;
			const float traversedSec		= START_SEC + iS * STEP_SIZE;
			const glm::vec4 samplePosWS	= biRay.At(traversedMain, traversedSec);
			const glm::vec4 samplePosBS	= worldSpaceToBiRaySpace * samplePosWS;

			float distanceWS;
			const glm::vec4 directionNWS	= mcm_Scene->EvaluateToSurfaceVector(samplePosWS, distanceWS);
			
			const glm::vec4 closestPosWS	= samplePosWS + distanceWS * directionNWS;
			const glm::vec4 closestPosBS	= worldSpaceToBiRaySpace * closestPosWS;

			const glm::vec4 directionNBS	= worldSpaceToBiRaySpace * directionNWS;

			const unsigned char distanceChar = 255 - static_cast<unsigned char>(distanceWS / DRAW_MAX_DISTANCE * 255.0f);
			unsigned char directionProjXChar = static_cast<unsigned char>(Math::Remap(-1, 1, 0, 255, -directionNWS.x));
			unsigned char directionProjYChar = static_cast<unsigned char>(Math::Remap(-1, 1, 0, 255, -directionNWS.y));
			unsigned char directionProjZChar = static_cast<unsigned char>(Math::Remap(-1, 1, 0, 255, -directionNWS.z));
			unsigned char directionProjWChar = static_cast<unsigned char>(Math::Remap(-1, 1, 0, 255, -directionNWS.w));

			if (traversedSec <= 0 && traversedSec >= -0.1f)
			{
				directionProjXChar = (int) (directionProjXChar * 0.5f);
				directionProjYChar = (int) (directionProjYChar * 0.5f);
				directionProjZChar = (int) (directionProjZChar * 0.5f);
				directionProjWChar = (int) (directionProjWChar * 0.5f);
			}

			if (distanceWS < mh_Configuration->RAY_HIT_EPSILON)
			{
				imageDistance.set_pixel(iM, iS, 0, 200, 50);
				imageCombined.set_pixel(iM, iS, 0, 200, 50);
				
				if (distanceWS < 0)
				{
					directionProjXChar	= (int) (directionProjXChar * 0.75f * (1.0f - distanceWS / DRAW_MAX_DISTANCE));
					directionProjYChar	= (int) (directionProjYChar * 0.75f * (1.0f - distanceWS / DRAW_MAX_DISTANCE));
					directionProjZChar	= (int) (directionProjZChar * 0.75f * (1.0f - distanceWS / DRAW_MAX_DISTANCE));
					directionProjWChar	= (int) (directionProjWChar * 0.75f * (1.0f - distanceWS / DRAW_MAX_DISTANCE));
				}
			}
			else
			{
				imageDistance.set_pixel(iM, iS, distanceChar, distanceChar, distanceChar);
				imageCombined.set_pixel(iM, iS, distanceChar, distanceChar, distanceChar);
			}
			 
			constexpr int MAGIC_VALUE_AVOID_OVERDRAW = 123;
			if (imageDirection.get_pixel(iM, iS).red == 255 && imageDirection.get_pixel(iM, iS).green == 0 && imageDirection.get_pixel(iM, iS).blue == 255)
			{
				// Avoid overdraw!
				unsigned int colorRed		= directionProjXChar;
				unsigned int colorGreen		= 0;
				unsigned int colorBlue		= directionProjYChar;

				if (distanceWS < mh_Configuration->RAY_HIT_EPSILON && distanceWS > -mh_Configuration->RAY_HIT_EPSILON)
				{
					const float fac	= 1.0f - Math::Abs(Math::Remap(0, mh_Configuration->RAY_HIT_EPSILON, 0.7f, 1.0f, distanceWS));				
					colorRed		= Math::Clamp(directionProjXChar + (int) (100.0f * fac), 0, 255);
					colorGreen		= (int) (255.0f * fac);	
					colorBlue		= Math::Clamp(directionProjYChar + (int) (200.0f * fac), 0, 255);
				}

				imageDirection.set_pixel(iM, iS, colorRed, colorGreen, colorBlue);
				imageCombined.set_pixel(iM, iS + GRID_SIZE, colorRed, colorGreen, colorBlue);
			}

			constexpr int FLOW_FIELD_SPACE = 20;
			constexpr float FLOW_FIELD_SIZE = 12.0f;
			if (iM % FLOW_FIELD_SPACE == 0 && iS % FLOW_FIELD_SPACE == 0)
			{
				const int offsetMain = static_cast<int>(std::round(directionNBS.z * FLOW_FIELD_SIZE));
				const int offsetSec	 = static_cast<int>(std::round(directionNBS.w * FLOW_FIELD_SIZE));
				
				const int x1 = iM;
				const int x2 = iM + offsetMain;

				const int y1 = iS;
				const int y2 = iS + offsetSec;
				
				if (distanceWS < mh_Configuration->RAY_HIT_EPSILON)
				{
					drawerDirection.pen_color(100, 100, MAGIC_VALUE_AVOID_OVERDRAW);
					drawerCombined.pen_color(100, 100, MAGIC_VALUE_AVOID_OVERDRAW);
				}
				else
				{
					drawerDirection.pen_color(255, 255, MAGIC_VALUE_AVOID_OVERDRAW);
					drawerCombined.pen_color(255, 255, MAGIC_VALUE_AVOID_OVERDRAW);
				}

				drawerDirection.line_segment(x1, y1, x2, y2);
				drawerCombined.line_segment(x1, y1 + GRID_SIZE, x2, y2 + GRID_SIZE);
				
				drawerDirection.pen_color(0, 0, MAGIC_VALUE_AVOID_OVERDRAW);
				drawerCombined.pen_color(0, 0, MAGIC_VALUE_AVOID_OVERDRAW);

				drawerDirection.circle(x1, y1, 1);
				drawerCombined.circle(x1, y1 + GRID_SIZE, 1);
			}
		}
	}

	// March Ray
	auto result2 = RayMarchFunctions::MarchSingleBiRay_DEBUG(biRay, biRaySpaceToWorldSpace, mcm_RenderSceneData, mh_Configuration->MIN_STEP_SIZE, mh_Configuration->MAX_DEPTH, mh_Configuration->MAX_STEPS, mh_Configuration->RAY_HIT_EPSILON, 
		START_MAIN, START_SEC, STEP_SIZE, GRID_SIZE, imagePath, imageCombined);
	
	if (result2.Hit)
	{
		const glm::vec4 toLightPosition	= mcm_Light->Position - result2.Position;
		const float toLightDistance	= glm::length(toLightPosition);
		const glm::vec4 toLightPositionN = toLightPosition / toLightDistance;

		// Shadow Ray
		const Math::Ray<glm::vec4> shadowRay	= Math::Ray<glm::vec4>(result2.Position + toLightPositionN * mh_Configuration->SHADOW_START_OFFSET, toLightPositionN);
		result2.ShadowValue						= RayMarchFunctions::MarchSecondaryShadowRay<glm::vec4>(shadowRay, mcm_RenderSceneData, toLightDistance, mcm_Light->Radius, mh_Configuration->MAX_STEPS_SHADOW, mh_Configuration->SHADOW_RAY_HIT_EPSILON, mh_Configuration->SHADOW_PENUMBRA);
	}

	const auto resultColor = VisualizationHelper::GetColorForRayResult(*mh_Configuration, result2);
	if (!result2.Hit)
	{
		for (int iX = 0; iX < GRID_SIZE; iX++)
		{
			for (int iY = 0; iY < GRID_SIZE; iY++)
			{
				constexpr int HARDCODED_RED = 200;
				if (imagePath.get_pixel(iX, iY).red == HARDCODED_RED)
				{
					imagePath.set_pixel(iX, iY, 220, 40, 40);
				}
			}
		}
	}

	// Draw Result Color Quad
	if (result2.Hit)
	{
		constexpr int SHOW_SIZE = 32;
		
		unsigned char normalXChar = static_cast<unsigned char>(Math::Remap(-1, 1, 0, 255, result2.Normal.x));
		unsigned char normalYChar = static_cast<unsigned char>(Math::Remap(-1, 1, 0, 255, result2.Normal.y));
		unsigned char normalZChar = static_cast<unsigned char>(Math::Remap(-1, 1, 0, 255, result2.Normal.z));
		unsigned char normalWChar = static_cast<unsigned char>(Math::Remap(-1, 1, 0, 255, result2.Normal.w));

		
		drawerCombined.pen_color(normalXChar, 0, normalZChar);
		drawerCombined.pen_width(5);
		for (int iX = 0; iX < SHOW_SIZE; iX ++)
		{
			for (int iY = 0; iY < SHOW_SIZE; iY++)
			{
				imageCombined.set_pixel(iX, GRID_SIZE - SHOW_SIZE + iY, normalXChar, 0, normalYChar);
			}
		}

		drawerCombined.pen_width(1);
		drawerCombined.pen_color(0, 0, 0);
		drawerCombined.line_segment(0, GRID_SIZE - SHOW_SIZE, SHOW_SIZE, GRID_SIZE - SHOW_SIZE);
		drawerCombined.line_segment(SHOW_SIZE, GRID_SIZE - SHOW_SIZE, SHOW_SIZE, GRID_SIZE);
	}

	// Save to file
	std::stringstream fileStream;
	std::tm timeinfo;
	localtime_s(&timeinfo, &m_ApplicationStartupTime);
	fileStream << "../Results/ProjectionDebug/" << std::put_time(&timeinfo, "%m-%d_%H-%M");
	
	std::filesystem::create_directories(fileStream.str().c_str());

	fileStream << "/" << id << "_" << (int) std::round(percentageView * 100.0f) << "_" << (int) std::round(percentageX * 10000.0f) << "_" << (int) std::round(percentageY * 10000.0f) << "_";
	auto basePath = fileStream.str();

	auto ssDist = std::stringstream();
	auto ssDir	= std::stringstream();
	auto ssPath = std::stringstream();
	auto ssComb = std::stringstream();
	ssDist	<< basePath << "distance.bmp";
	ssDir	<< basePath << "direction.bmp";
	ssPath	<< basePath << "path.bmp";
	ssComb	<< basePath << "combined.bmp";

	imageDistance.save_image(ssDist.str());
	imageDirection.save_image(ssDir.str());
	imagePath.save_image(ssPath.str());
	imageCombined.save_image(ssComb.str());
	
	RayMarchResult<glm::vec4> result3 = RayMarchFunctions::MarchSingleBiRay<DimensionVector, DimensionMatrix>(biRay, biRaySpaceToWorldSpace, mcm_RenderSceneData, mh_Configuration->MIN_STEP_SIZE, mh_Configuration->MAX_DEPTH, mh_Configuration->MAX_STEPS, mh_Configuration->RAY_HIT_EPSILON);
	printf("Result for percentage %f, %f: HitDEBUG %hs, HitREAL %hs, Color %i %i %i %i", percentageX, percentageY, result2.Hit ? "yessa!" : "nah my dude :/", result2.Hit ? "yessa!" : "nah my dude :/", resultColor.Red, resultColor.Green, resultColor.Blue, resultColor.Alpha);
	printf("Sampling Done");
}

A_CUDA_CPU void Application::MainLoop()
{
	//////////////////////////////////////////////////////////////////////////
	// 1) Setup buffers, threads	

	QuiltConfiguration initializedQuiltConfiguration = ActiveQuiltConfiguration;

	std::future<void> exitFuture = m_RaymarchThreadExitSignal.get_future();
	m_RaymarchThread = std::thread(&RenderingThread, this, std::move(exitFuture));

	mt_IsWritingIntoBuffer[1] = true;
	CUDA_PrepareRenderImage(m_RenderingBuffers[1], md_RenderingSurfaceObject);		
	mcm_Scene->Update(*mh_Configuration);
	mt_StartRender = true;

	// Wait for first render
	while (mt_IsWritingIntoBuffer[1])
	{
		std::this_thread::yield();
	}
	
	GL_CHECK_ERROR();

	BeginImgui();
	UpdateManagers();
	EndImgui();	

	//////////////////////////////////////////////////////////////////////////
	// Main Loop

	while (!glfwWindowShouldClose(m_Window))
	{
		unsigned int nextBufferID = (m_RenderingBufferIDMainThread + 1) % BUFFER_COUNT;

		// Check for resolution changes
		const bool quiltConfigurationChanged = initializedQuiltConfiguration != ActiveQuiltConfiguration;
		if (quiltConfigurationChanged)
		{
			// Wait for all active renderings to finish.
			for (unsigned int i = 0; i < BUFFER_COUNT; i++)
			{
				while (mt_IsWritingIntoBuffer[i])
				{
					continue;
				}
			}
			
			// Rebuild textures and buffers, restart at
			ReInitRendering(ActiveQuiltConfiguration);
			initializedQuiltConfiguration = ActiveQuiltConfiguration;
			std::this_thread::yield();

			continue;
		}

		//////////////////////////////////////////////////////////////////////////
		// Check if a frame is ready to be uploaded and then displayed

		bool nextFrameDone = !mt_IsWritingIntoBuffer[nextBufferID];
		if (nextFrameDone)
		{
			// 1) Free resources for finished frame
			CUDA_FinishRenderImage(m_RenderingBuffers[nextBufferID]);
			m_RenderingBufferIDMainThread = nextBufferID;
			
			// 2) Update Scene
			ApplyCameraInput();
			ApplyLightInput();
			mcm_Scene->Update(*mh_Configuration);

			// 3) Lock resources for future frame
			CUDA_PrepareRenderImage(m_RenderingBuffers[m_RenderingBufferIDRaymarchingThread], md_RenderingSurfaceObject);
			
			// 4) Signal raymarching thread
			mt_StartRender = true;

			GL_CHECK_ERROR();
		}

		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		
		// 1) Input
		glfwPollEvents();
		ProcessInput();

		// 2) Render
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		DrawScreenQuad();

		BeginImgui();
		UpdateManagers();
		EndImgui();

		// 3) Swap Chain
		glfwSwapBuffers(m_Window);
		
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		m_LastMainLoopTimeMyS = std::chrono::duration_cast<std::chrono::microseconds>(end - begin);
		
		GL_CHECK_ERROR();
	}

	//////////////////////////////////////////////////////////////////////////

	// Signal rm thread to stop.
	m_RaymarchThreadExitSignal.set_value();

	// Wait for rm thread.
	m_RaymarchThread.join();

	//////////////////////////////////////////////////////////////////////////
}

A_CUDA_CPU void Application::ProcessInput()
{
	if (glfwGetKey(m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_Window, true);
	}

	ProcessCameraInput();

	if (glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS)
	{
		double xPos, yPos;
		glfwGetCursorPos(m_Window, &xPos, &yPos);
		
		double percentageX = xPos / m_WindowSize.x;
		double percentageY = 1.0f - yPos / m_WindowSize.y;

		if (glfwGetKey(m_Window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		{
			g_DebugMarchRay = true;
			g_DebugMarchRayPercentage = glm::vec2(static_cast<float>(percentageX), static_cast<float>(percentageY));

			if (glfwGetKey(m_Window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			{
				g_DebugMarchRayMulti = true;
			}
		}
	}

	if (glfwGetKey(m_Window, GLFW_KEY_0) == GLFW_PRESS)
	{
		g_DebugReMarchRay = true;
	}

	//////////////////////////////////////////////////////////////////////////
	
	if (glfwGetKey(m_Window, GLFW_KEY_O) == GLFW_PRESS)
	{
		printf("Camera Config: \n");
		printf("	configuration->SceneSliderRotations[0] = %.3ff; \n",		mh_Configuration->SceneSliderRotations[0]);
		printf("	configuration->SceneSliderRotations[1] = %.3ff; \n",		mh_Configuration->SceneSliderRotations[1]);
		printf("	configuration->SceneSliderRotations[2] = %.3ff; \n",		mh_Configuration->SceneSliderRotations[2]);
		printf("	configuration->SceneSliderRotations[3] = %.3ff; \n",		mh_Configuration->SceneSliderRotations[3]);
		printf("	configuration->SceneSliderRotations[4] = %.3ff; \n",		mh_Configuration->SceneSliderRotations[4]);
		printf("	configuration->SceneSliderRotations[5] = %.3ff; \n\n",		mh_Configuration->SceneSliderRotations[5]);
	}
}

//////////////////////////////////////////////////////////////////////////

A_CUDA_CPU void Application::ProcessCameraInput()
{
	//////////////////////////////////////////////////////////////////////////
	// Camera

	if (glfwGetKey(m_Window, GLFW_KEY_R) == GLFW_PRESS)		g_KeyR	= true;
	if (glfwGetKey(m_Window, GLFW_KEY_R) == GLFW_RELEASE)	g_KeyR	= false;
	
	if (g_KeyR)
	{
		m_CameraAngleZW = CAMERA_ANGLE_ZW_DEFAULT;
		m_CameraAngleYZ = 0;
		m_CameraAngleXY = 0;
		//resetMove = true;
	}
	
	constexpr float CAMERA_SPEED_180 = 0.015f;
	constexpr float CAMERA_SPEED_360 = 0.060f;
	
	if (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS)		g_KeyA	= true;
	if (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_RELEASE)	g_KeyA	= false;
	if (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS)		g_KeyD	= true;
	if (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_RELEASE)	g_KeyD	= false;
	if (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS)		g_KeyW	= true;
	if (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_RELEASE)	g_KeyW	= false;
	if (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS)		g_KeyS	= true;
	if (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_RELEASE)	g_KeyS	= false;
	if (glfwGetKey(m_Window, GLFW_KEY_Q) == GLFW_PRESS)		g_KeyQ	= true;
	if (glfwGetKey(m_Window, GLFW_KEY_Q) == GLFW_RELEASE)	g_KeyQ	= false;
	if (glfwGetKey(m_Window, GLFW_KEY_E) == GLFW_PRESS)		g_KeyE	= true;
	if (glfwGetKey(m_Window, GLFW_KEY_E) == GLFW_RELEASE)	g_KeyE	= false;
	if (glfwGetKey(m_Window, GLFW_KEY_Y) == GLFW_PRESS)		g_KeyY	= true;
	if (glfwGetKey(m_Window, GLFW_KEY_Y) == GLFW_RELEASE)	g_KeyY	= false;
	//if (glfwGetKey(m_Window, GLFW_KEY_X) == GLFW_PRESS)		g_KeyX	= true;
	//if (glfwGetKey(m_Window, GLFW_KEY_X) == GLFW_RELEASE)	g_KeyX	= false;
	
	if (!(g_KeyD || g_KeyA || g_KeyW || g_KeyS || g_KeyE || g_KeyQ /* || g_KeyX || g_KeyY */))
	{
		return;
	}

	constexpr bool ALLOW_HYPERSPHERE_CAMERA = false;
	if (!ALLOW_HYPERSPHERE_CAMERA)
	{
		// For now, camera movement on the hypersphere is disabled by this early return.
		return;
	}
	
	const float moveZW = CAMERA_SPEED_180 * (static_cast<int>(g_KeyE) - static_cast<int>(g_KeyQ));
	const float moveYZ = CAMERA_SPEED_180 * (static_cast<int>(g_KeyW) - static_cast<int>(g_KeyS));
	const float moveXY = CAMERA_SPEED_360 * (static_cast<int>(g_KeyD) - static_cast<int>(g_KeyA));

	m_CameraAngleZW = m_CameraAngleZW + moveZW; //Math::Clamp(m_CameraAngleZW + moveZW, - glm::pi<float>() / 2.0f, + glm::pi<float>() / 2.0f);
	m_CameraAngleYZ = m_CameraAngleYZ + moveYZ; //Math::Clamp(m_CameraAngleYZ + moveYZ, 0.0f, glm::pi<float>());
	m_CameraAngleXY = m_CameraAngleXY + moveXY; //Math::Clamp(m_CameraAngleXY + moveXY, 0.0f, 2.0f * glm::pi<float>());
}

//////////////////////////////////////////////////////////////////////////

int lastSampleID = 0;
float lastSamplePercentageX = 0.0f;
float lastSamplePercentageY = 0.0f;
float lastSampleViewPercentage = 0.0f;

void Application::ApplyCameraInput()
{
	mcm_Camera->PrimaryProjectionMethod		= m_DesiredCameraProjectionMethodMain;
	mcm_Camera->SecondaryProjectionMethod	= m_DesiredCameraProjectionMethodSecondary;
	
	mcm_Camera->ViewPanePrimarySizeFactor	= m_DesiredCameraPaneScaleZ;
	mcm_Camera->ViewPaneSecondarySizeFactor	= m_DesiredCameraPaneScaleW;

	if (g_DebugReMarchRay)
	{
		MarchSample(lastSampleID, lastSamplePercentageX, lastSamplePercentageY, lastSampleViewPercentage);
		g_DebugReMarchRay = false;
	}

	static int id = 0;
	if (g_DebugMarchRay)
	{
		g_DebugMarchRay				= false;

		if (!g_DebugMarchRayMulti)
		{
			lastSampleID				= id;
			lastSamplePercentageX		= g_DebugMarchRayPercentage.x;
			lastSamplePercentageY		= g_DebugMarchRayPercentage.y;
			lastSampleViewPercentage	= 0.5f;

			MarchSample(id, g_DebugMarchRayPercentage.x, g_DebugMarchRayPercentage.y, 0.5f);
			id++;
		}
		else
		{
			constexpr int VIEWS = 4*8;
			for (int i = 0; i < VIEWS; i++)
			{
				MarchSample(id, g_DebugMarchRayPercentage.x, g_DebugMarchRayPercentage.y, i * 1.0f / VIEWS);
			}
			id++;
		}
	}

	//////////////////////////////////////////////////////////////////////////

	mcm_Camera->UpdatePosition(m_CameraAngleZW, m_CameraAngleYZ, m_CameraAngleXY);

	m_LastCameraPosition	= mcm_Camera->GetPosition();
	m_LastCameraRight		= mcm_Camera->RightVector;
	m_LastCameraUp			= mcm_Camera->UpVector;
	m_LastCameraForward		= mcm_Camera->ForwardVector;
	m_LastCameraOver		= mcm_Camera->OverVector;
}

//////////////////////////////////////////////////////////////////////////

void Application::ApplyLightInput()
{
	mcm_Light->Position = m_DesiredLightPosition;
	mcm_Light->Radius	= m_DesiredLightRadius;
}

//////////////////////////////////////////////////////////////////////////

void Application::RenderingThread(Application* application, std::future<void> exitFuture)
{
	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	// INSIDE OF RENDERING THREAD

	application->m_RenderingBufferIDRaymarchingThread = 1;
	application->mcm_RenderSceneData->Initialize(application->mcm_Scene);
	
	// Loop while we do not want to quit.
	while (exitFuture.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
	{
		// 1) Wait for StartRender
		if (!application->mt_StartRender)
		{
			continue;
		}
		
		//////////////////////////////////////////////////////////////////////////
		// 2) Prepare render parameters

		application->mt_StartRender = false;
		application->mt_IsWritingIntoBuffer[application->m_RenderingBufferIDRaymarchingThread] = true;
			
		application->mcm_RenderBufferData->Initialize(
			application->md_RenderingSurfaceObject,
			application->m_QuiltConfigData.UsedTextureDimensions, 
			application->m_QuiltConfigData.ViewDimensions, 
			application->m_QuiltConfigData.Views);
		
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		cudaMemcpy(application->md_Configuration, application->mh_Configuration, sizeof(Configuration), cudaMemcpyHostToDevice);
		
		// 3) Wait for Render
		CUDA_RenderImage(application->mcm_RenderBufferData, application->mcm_RenderSceneData, application->md_Configuration, application->mcm_Camera, application->mcm_Light);
		
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		application->m_LastRayMarchingTimeMyS = std::chrono::duration_cast<std::chrono::microseconds>(end - begin);

		// 4) Signal main thread to unlock resources and swap buffers
		application->mt_IsWritingIntoBuffer[application->m_RenderingBufferIDRaymarchingThread] = false;
		
		application->m_RenderingBufferIDRaymarchingThread = (application->m_RenderingBufferIDRaymarchingThread + 1) % BUFFER_COUNT;

	}

	// END OF RAYMARCHING THREAD
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
}

//////////////////////////////////////////////////////////////////////////

void Application::TestOpenGlError(const char* file, unsigned int line) const
{
	GLenum errorCode = glGetError();

	while (errorCode != GL_NO_ERROR) 
	{
		//std::string fileString(file);
		std::string error = "unknown error";

		// clang-format off
		switch (errorCode) {
			case GL_INVALID_ENUM:      error = "GL_INVALID_ENUM"; break;
			case GL_INVALID_VALUE:     error = "GL_INVALID_VALUE"; break;
			case GL_INVALID_OPERATION: error = "GL_INVALID_OPERATION"; break;
			case GL_STACK_OVERFLOW:    error = "GL_STACK_OVERFLOW"; break;
			case GL_STACK_UNDERFLOW:   error = "GL_STACK_UNDERFLOW"; break;
			case GL_OUT_OF_MEMORY:     error = "GL_OUT_OF_MEMORY"; break;
		}
		// clang-format on

		std::cerr << "OpenglError : file=" << file << " line=" << line << " error:" << error << std::endl;
		errorCode = glGetError();
	}
}

//////////////////////////////////////////////////////////////////////////

void FrameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

//////////////////////////////////////////////////////////////////////////

static void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
		type, severity, message);
}

//////////////////////////////////////////////////////////////////////////

void Application::DrawScreenQuad() const
{
	GL_CHECK_ERROR();
	
	if (m_RenderingBuffers[m_RenderingBufferIDMainThread].IsCurrentlyMapped)
	{
		std::cerr << "Still mapped";
	}

	ShaderUtility::BindShader(m_HPLightFieldShaderHandle);
	ShaderUtility::SetUniform(m_HPLightFieldShaderHandle, "debug", DrawQuiltInsteadOfLightfield);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_RenderingBuffers[m_RenderingBufferIDMainThread].TextureHandle);
	GL_CHECK_ERROR();
	glBindVertexArray(m_ScreenQuadVAOHandle);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	ShaderUtility::UnBindShader();
	
	GL_CHECK_ERROR();
}

//////////////////////////////////////////////////////////////////////////

void Application::InitImgui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;		// Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	
	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
	ImGui_ImplOpenGL3_Init(Application::GLSL_VERSION_IMGUI);

	// No additional fonts so far.
	ImFontConfig config;
	config.SizePixels = 26;
	io.Fonts->AddFontDefault(&config);
}

void Application::CleanupImgui()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

//////////////////////////////////////////////////////////////////////////

void Application::InitScene()
{
	mcm_Scene = new SceneHyperPlayground();
	mcm_Scene->Init();

}

//////////////////////////////////////////////////////////////////////////

void Application::CleanupScene()
{
	delete mcm_Scene;
}

//////////////////////////////////////////////////////////////////////////

void Application::InitBaseShaders()
{
	// 1) Load blit shader
	m_HPBlitShaderHandle = ShaderUtility::LoadUpShader("Shaders/HoloPlayBlit.vert", "Shaders/HoloPlayBlit.frag");
	GL_CHECK_ERROR();
}

//////////////////////////////////////////////////////////////////////////

void Application::UpdateManagers()
{
	m_OptionsManager->DrawDebugWindows(*this, *mh_Configuration);
	m_StudyManager->Update(*this, *mh_Configuration);

}

//////////////////////////////////////////////////////////////////////////

void Application::BeginImgui()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

//////////////////////////////////////////////////////////////////////////

void Application::EndImgui()
{
	// Rendering
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Update and Render additional Platform Windows
	// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
	//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
}

//////////////////////////////////////////////////////////////////////////
// Adapted from HoloPlay sample app.

void Application::InitForNormalDisplay()
{
	m_WindowPosition.x	= 1000;
	m_WindowPosition.y	= 1000;
	m_WindowSize.x		= 1600;
	m_WindowSize.y		= 900;
}

//////////////////////////////////////////////////////////////////////////

bool Application::InitLookingGlass()
{
	auto errorHandler = [this]()
	{
		//InitForNormalDisplay();
		hpc_TeardownMessagePipe();
		std::cerr << "Couldn't find looking glass" << std::endl;
		
		return false;
	};

	//////////////////////////////////////////////////////////////////////////

	printf("Initializing Looking Glass");

	// Init App
	hpc_client_error errorCode = hpc_InitializeApp("Aurora", hpc_LICENSE_NONCOMMERCIAL);
	if (errorCode) 
	{
		const char* errstr;
		switch (errorCode) 
		{
			case hpc_CLIERR_NOSERVICE:
			errstr = "HoloPlay Service not running";
			break;
			case hpc_CLIERR_SERIALIZEERR:
			errstr = "Client message could not be serialized";
			break;
			case hpc_CLIERR_VERSIONERR:
			errstr = "Incompatible version of HoloPlay Service";
			break;
			case hpc_CLIERR_PIPEERROR:
			errstr = "Interprocess pipe broken";
			break;
			case hpc_CLIERR_SENDTIMEOUT:
			errstr = "Interprocess pipe send timeout";
			break;
			case hpc_CLIERR_RECVTIMEOUT:
			errstr = "Interprocess pipe receive timeout";
			break;
			default:
			errstr = "Unknown error";
			break;
		}

		printf("Client access error (code = %d): %s!\n", errorCode, errstr);
		
		return errorHandler();
	} 

	char buf[1000];

	hpc_GetHoloPlayCoreVersion(buf, 1000);
	printf("HoloPlay Core version %s.\n", buf);
	hpc_GetHoloPlayServiceVersion(buf, 1000);
	printf("HoloPlay Service version %s.\n", buf);
	int num_displays = hpc_GetNumDevices();
	printf("%d device%s connected.\n", num_displays,
			(num_displays == 1 ? "" : "s"));

	if (num_displays < 1)
	{
		return errorHandler();
	}

	// Print device info

	for (int i = 0; i < num_displays; ++i) {
		printf("Device information for display %d:\n", i);
		hpc_GetDeviceHDMIName(i, buf, 1000);
		printf("\tDevice name: %s\n", buf);
		hpc_GetDeviceType(i, buf, 1000);
		printf("\tDevice type: %s\n", buf);
		printf("\nWindow parameters for display %d:\n", i);
		printf("\tPosition: (%d, %d)\n", hpc_GetDevicePropertyWinX(i), hpc_GetDevicePropertyWinY(i));
		printf("\tSize: (%d, %d)\n", hpc_GetDevicePropertyScreenW(i), hpc_GetDevicePropertyScreenH(i));
		printf("\tAspect ratio: %f\n", hpc_GetDevicePropertyDisplayAspect(i));
		printf("Shader uniforms for display %d:\n", i);
		printf("\tpitch: %.9f\n", hpc_GetDevicePropertyPitch(i));
		printf("\ttilt: %.9f\n", hpc_GetDevicePropertyTilt(i));
		printf("\tcenter: %.9f\n", hpc_GetDevicePropertyCenter(i));
		printf("\tsubp: %.9f\n", hpc_GetDevicePropertySubp(i));
		printf("\tviewCone: %.1f\n", hpc_GetDevicePropertyFloat(i, "/calibration/viewCone/value"));
		printf("\tfringe: %.1f\n", hpc_GetDevicePropertyFringe(i));
		printf("\tRI: %d\n \tBI: %d\n \tinvView: %d\n", hpc_GetDevicePropertyRi(i), hpc_GetDevicePropertyBi(i), hpc_GetDevicePropertyInvView(i));
	}
	
	// Get dimensions & origin of looking glass
	m_WindowSize.x		= hpc_GetDevicePropertyScreenW(DEV_INDEX);
	m_WindowSize.y		= hpc_GetDevicePropertyScreenH(DEV_INDEX);
	m_WindowPosition.x	= hpc_GetDevicePropertyWinX(DEV_INDEX);
	m_WindowPosition.y	= hpc_GetDevicePropertyWinY(DEV_INDEX);

	return true;
}

//////////////////////////////////////////////////////////////////////////

void Application::CleanupLookingGlass()
{
	hpc_TeardownMessagePipe();
}

//////////////////////////////////////////////////////////////////////////
// Adapted from HoloPlayCore Example project

void Application::SetupLookingGlass()
{
	//////////////////////////////////////////////////////////////////////////
	// 1) Load lightfield shader
	const char* openGLVersion = "#version 330 core\n";

	std::stringstream stringStreamVert, stringStreamFrag;
	stringStreamVert << openGLVersion << hpc_LightfieldVertShaderGLSL;
	stringStreamFrag << openGLVersion << hpc_LightfieldFragShaderGLSL;

	m_HPLightFieldShaderHandle = ShaderUtility::LoadUpShaderCode(stringStreamVert.str().c_str(), stringStreamFrag.str().c_str());
	GL_CHECK_ERROR();

	//////////////////////////////////////////////////////////////////////////
	// 2) Load Calibration into shader

	ShaderUtility::BindShader(m_HPLightFieldShaderHandle);
	ShaderUtility::SetUniform(m_HPLightFieldShaderHandle, "pitch",			hpc_GetDevicePropertyPitch(DEV_INDEX));
	ShaderUtility::SetUniform(m_HPLightFieldShaderHandle, "tilt",			hpc_GetDevicePropertyTilt(DEV_INDEX));
	ShaderUtility::SetUniform(m_HPLightFieldShaderHandle, "center",			hpc_GetDevicePropertyCenter(DEV_INDEX));
	ShaderUtility::SetUniform(m_HPLightFieldShaderHandle, "invView",		hpc_GetDevicePropertyInvView(DEV_INDEX));
	ShaderUtility::SetUniform(m_HPLightFieldShaderHandle, "quiltInvert",	0);
	ShaderUtility::SetUniform(m_HPLightFieldShaderHandle, "subp",			hpc_GetDevicePropertySubp(DEV_INDEX));
	ShaderUtility::SetUniform(m_HPLightFieldShaderHandle, "ri",				hpc_GetDevicePropertyRi(DEV_INDEX));
	ShaderUtility::SetUniform(m_HPLightFieldShaderHandle, "bi",				hpc_GetDevicePropertyBi(DEV_INDEX));
	ShaderUtility::SetUniform(m_HPLightFieldShaderHandle, "displayAspect",	hpc_GetDevicePropertyDisplayAspect(DEV_INDEX));
	ShaderUtility::SetUniform(m_HPLightFieldShaderHandle, "quiltAspect",	hpc_GetDevicePropertyDisplayAspect(DEV_INDEX));
	ShaderUtility::SetUniform(m_HPLightFieldShaderHandle, "screenTex",		0);	// < We always render texture unit 0, which is where we will store the current texture.
	ShaderUtility::UnBindShader();
	GL_CHECK_ERROR();

	//////////////////////////////////////////////////////////////////////////
	// 3) Quilts
	 
	ConfigureQuilt(_2k_4x8);
	PassQuiltSettingsToShader();
}

//////////////////////////////////////////////////////////////////////////

void Application::SetupForNormalDisplay()
{
	m_HPLightFieldShaderHandle = m_HPBlitShaderHandle;

	ConfigureQuilt(_2k_1x1);
}

//////////////////////////////////////////////////////////////////////////
 
void Application::ConfigureQuilt(QuiltConfiguration option)
{
	ActiveQuiltConfiguration = option;
	switch (option)
	{	
		case _16_singleView: m_QuiltConfigData.Initialize({16, 16}, {1, 1}, {16, 16}); return;			//  512 x 512 px for the single view
		case _512_singleView: m_QuiltConfigData.Initialize({512, 512}, {1, 1}, {16, 16}); return;		//  512 x 512 px for the single view
		case _512_2x4: m_QuiltConfigData.Initialize({512, 512}, {2, 4}, {16, 16}); return;				//  256 x 128 px per view
		case _512_4x8: m_QuiltConfigData.Initialize({512, 512}, {4, 8}, {16, 16}); return;				//  128 x 064 px per view
		case _1k_4x8: m_QuiltConfigData.Initialize({1024, 1024}, {4, 8}, {16, 16}); return;				//  512 x 256 px per view
		case _2k_4x8: m_QuiltConfigData.Initialize({2048, 2048}, {4, 8}, {16, 16}); return;				//  512 x 256 px per view
		case _2k_1x1: m_QuiltConfigData.Initialize({2048, 2048}, {1, 1}, {16, 16}); return;				//  512 x 256 px per view
		case _4k_5x9: m_QuiltConfigData.Initialize({4096, 4096}, {5, 9}, {13, 13}); return;				//  819 x 455 px per view
		case _8k_5x9: m_QuiltConfigData.Initialize({8192, 8192}, {5, 9}, {13, 13}); return;				// 1638 x 910 px per view	
	}

	m_QuiltConfigData.Initialize({1,1}, {1,1}, {1, 1});
	throw std::exception("Invalid Quit Setting");
}

//////////////////////////////////////////////////////////////////////////

void Application::PassQuiltSettingsToShader() const
{
	ShaderUtility::BindShader(m_HPLightFieldShaderHandle);
	ShaderUtility::SetUniform(m_HPLightFieldShaderHandle, "overscan",		0);
	ShaderUtility::SetUniform(m_HPLightFieldShaderHandle, "tile",			glm::vec3(m_QuiltConfigData.Views.x, m_QuiltConfigData.Views.y, m_QuiltConfigData.ViewCount));
	
	const float viewPortionX = m_QuiltConfigData.UsedTextureDimensions.x / static_cast<float>(m_QuiltConfigData.TextureDimensions.x);
	const float viewPortionY = m_QuiltConfigData.UsedTextureDimensions.y / static_cast<float>(m_QuiltConfigData.TextureDimensions.y);
	ShaderUtility::SetUniform(m_HPLightFieldShaderHandle, "viewPortion",	glm::vec2(viewPortionX, viewPortionY));
	ShaderUtility::UnBindShader();
}