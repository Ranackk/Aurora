#include "stdafx.h"

#include "Options/OptionsManager.h"

#include "Rendering/Application.h"
#include "Rendering/QuiltTypes.h"
#include "glm/ext/scalar_constants.hpp"

void OptionsManager::DrawDebugWindows(Application& application, Configuration& config)
{
	const ImVec2 windowPosStats		= {20,  120};
	
	//////////////////////////////////////////////////////////////////////////

	if (m_FirstTick) 
	{
		ImGui::SetNextWindowPos(windowPosStats);
		ImGui::SetNextWindowSize({700, 1000});
	}
	
	ImGui::Begin("Debug");
	if (ImGui::BeginTabBar("Tab"))
	{
		if (ImGui::BeginTabItem("Stats"))
		{
			ImGui::Text("FrameTime    [ms]: %04.2f | %i frames per second",	Application::Instance()->LastMainLoopTimeMyS().count() / 1000.0f, static_cast<int>(std::round(1000000.0f / Application::Instance()->LastMainLoopTimeMyS().count())));
			ImGui::Text("RayMarchTime [ms]: %04.2f | %i render per second",	Application::Instance()->LastRayMarchingTimeMyS().count() / 1000.0f, static_cast<int>(std::round(1000000.0f / Application::Instance()->LastRayMarchingTimeMyS().count())));

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Renderer"))
		{
			ImGui::Checkbox("Draw Quilt", &application.DrawQuiltInsteadOfLightfield);
	
			int activeQuiltConfiguration = static_cast<int>(application.ActiveQuiltConfiguration);
			ImGui::Combo("Quilt Configuration", &activeQuiltConfiguration, s_QuiltConfigurationsNames, static_cast<int>(QuiltConfiguration::Count));
			if (activeQuiltConfiguration != static_cast<int>(application.ActiveQuiltConfiguration))
			{
				application.ActiveQuiltConfiguration = static_cast<QuiltConfiguration>(activeQuiltConfiguration);
			}

		#ifdef _DEBUG

			ImGui::InputInt("Max Steps",				&config.MAX_STEPS);
			ImGui::InputFloat("Max Depth",				&config.MAX_DEPTH);
			ImGui::InputFloat("Ray Hit Epsilon",		&config.RAY_HIT_EPSILON, 0.005f, 0.015f);
			ImGui::InputFloat("Checkerboard Size",		&config.CHECKERBOARD_SIZE, 0.25f, 1.5f);

			ImGui::Spacing();
		#endif // DEBUG

			ImGui::Text("Visualize Rays");
			int drawModeHit		= static_cast<int>(config.DrawModeHit);
			ImGui::Combo("On Hit", &drawModeHit, config.s_DrawModeNames, static_cast<int>(Configuration::DrawMode::Count));
			if (drawModeHit != static_cast<int>(config.DrawModeHit))
			{
				config.DrawModeHit	= static_cast<Configuration::DrawMode>(drawModeHit);
			}

			if (config.DrawModeHit == Configuration::DrawMode::SolidColor)
			{	
				ImGui::ColorPicker4("ColorHit", &config.DrawModeSolidColorHit.Value.x);
			}

			int drawModeMiss	= static_cast<int>(config.DrawModeMiss);
			ImGui::Combo("On Miss", &drawModeMiss, config.s_DrawModeNames, static_cast<int>(Configuration::DrawMode::Count));
			if (drawModeMiss != static_cast<int>(config.DrawModeMiss))
			{
				config.DrawModeMiss	= static_cast<Configuration::DrawMode>(drawModeMiss);
			}
	
			if (config.DrawModeMiss == Configuration::DrawMode::SolidColor)
			{
				ImGui::ColorPicker4("ColorMiss", &config.DrawModeSolidColorMiss.Value.x);
			}
	
			if (config.DrawModeHit == Configuration::DrawMode::NormalConfigurable || config.DrawModeMiss == Configuration::DrawMode::NormalConfigurable || 
				config.DrawModeHit == Configuration::DrawMode::Position || config.DrawModeMiss == Configuration::DrawMode::Position ||
				config.DrawModeHit == Configuration::DrawMode::LocalPosition || config.DrawModeMiss == Configuration::DrawMode::LocalPosition)
			{	
				constexpr int axisCount = DimensionVector::length();
				ImGui::Combo("red",		&config.DrawModeConfigurableColorByAxisIDs[0], Configuration::s_AxisNames, axisCount + 1);
				ImGui::Combo("green",	&config.DrawModeConfigurableColorByAxisIDs[1], Configuration::s_AxisNames, axisCount + 1);
				ImGui::Combo("blue",	&config.DrawModeConfigurableColorByAxisIDs[2], Configuration::s_AxisNames, axisCount + 1);
				ImGui::Combo("alpha",	&config.DrawModeConfigurableColorByAxisIDs[3], Configuration::s_AxisNames, axisCount + 1);
			}

			if (config.DrawModeHit == Configuration::DrawMode::Position || config.DrawModeMiss == Configuration::DrawMode::Position ||
				config.DrawModeHit == Configuration::DrawMode::LocalPosition || config.DrawModeMiss == Configuration::DrawMode::LocalPosition)
			{
				ImGui::DragFloat4("PositionMin", &config.DrawModePositionMin[0]);
				ImGui::DragFloat4("PositionMax", &config.DrawModePositionMax[0]);
			}

			int idCameraProjectionMain = static_cast<int>(application.m_DesiredCameraProjectionMethodMain);
			ImGui::Combo("Projection Z", &idCameraProjectionMain, s_ProjectionMethodNames, 2);
			if (idCameraProjectionMain != static_cast<int>(application.m_DesiredCameraProjectionMethodMain))
			{
				application.m_DesiredCameraProjectionMethodMain = static_cast<ProjectionMethod>(idCameraProjectionMain);
			}

			int idCameraProjectionSecondary = static_cast<int>(application.m_DesiredCameraProjectionMethodSecondary);
			ImGui::Combo("Projection W", &idCameraProjectionSecondary, s_ProjectionMethodNames, 2);
			if (idCameraProjectionSecondary != static_cast<int>(application.m_DesiredCameraProjectionMethodSecondary))
			{
				application.m_DesiredCameraProjectionMethodSecondary = static_cast<ProjectionMethod>(idCameraProjectionSecondary);
			}
	
			ImGui::SliderFloat("ScaleFactor FOV Z", &application.m_DesiredCameraPaneScaleZ, 0.01f, 10.0f);
			ImGui::SliderFloat("ScaleFactor FOV W", &application.m_DesiredCameraPaneScaleW, 0.01f, 10.0f);
	
			ImGui::SliderFloat4("Light Position", &application.m_DesiredLightPosition.x, -150.0f, 150.0f);
			ImGui::SliderFloat("Light Radius", &application.m_DesiredLightRadius, 1.0f, 1000.0f);
			ImGui::EndTabItem();
		}
		
		if (ImGui::BeginTabItem("Scene"))
		{
			ImGui::Text("Scene Object Rotation Sliders");

		ImGui::SliderFloat("Rot ZW (z-Axis)",	&config.SceneSliderRotations[0], -4.0f * glm::pi<float>(), 4.0f * glm::pi<float>());
		ImGui::Checkbox("Animate ZW", &config.SceneAnimateRotations[0]);
		ImGui::SliderFloat("Rot YW (-y-Axis)",	&config.SceneSliderRotations[1], -4.0f * glm::pi<float>(), 4.0f * glm::pi<float>());
		ImGui::Checkbox("Animate YW", &config.SceneAnimateRotations[1]);
		ImGui::SliderFloat("Rot YZ /",			&config.SceneSliderRotations[2], -4.0f * glm::pi<float>(), 4.0f * glm::pi<float>());
		ImGui::Checkbox("Animate YZ", &config.SceneAnimateRotations[2]);
		ImGui::SliderFloat("Rot XW (x-Axis)",	&config.SceneSliderRotations[3], -4.0f * glm::pi<float>(), 4.0f * glm::pi<float>());
		ImGui::Checkbox("Animate XW", &config.SceneAnimateRotations[3]);
		ImGui::SliderFloat("Rot XZ /",			&config.SceneSliderRotations[4], -4.0f * glm::pi<float>(), 4.0f * glm::pi<float>());
		ImGui::Checkbox("Animate XZ", &config.SceneAnimateRotations[4]);
		ImGui::SliderFloat("Rot XY /",			&config.SceneSliderRotations[5], -4.0f * glm::pi<float>(), 4.0f * glm::pi<float>());
		ImGui::Checkbox("Animate XY", &config.SceneAnimateRotations[5]);
		
		if (config.SceneAnimateRotations[0] || config.SceneAnimateRotations[1] || config.SceneAnimateRotations[2] || config.SceneAnimateRotations[3] || config.SceneAnimateRotations[4] || config.SceneAnimateRotations[5])
		{
			ImGui::SliderFloat("Animation Speed", &config.SceneSpeed, 0.0f, 5.0f);
		}

		if (ImGui::Button("Reset Rotation"))
		{
			for (int i = 0; i < 6; i++) 
			{
				config.SceneSliderRotations[i] = 0.0f;
			}
		}
		
		if (ImGui::Button("Preset 1"))
		{
			config.SceneSliderRotations[0] = 0.785f;
			config.SceneSliderRotations[1] = 0.785f;
			config.SceneSliderRotations[2] = 0.785f;
			config.SceneSliderRotations[3] = 0.0f;
			config.SceneSliderRotations[4] = 0.0f;
			config.SceneSliderRotations[5] = 0.0f;
		}
		if (ImGui::Button("Preset 2"))
		{
			config.SceneSliderRotations[0] = 0.785f;
			config.SceneSliderRotations[1] = 0.785f;
			config.SceneSliderRotations[2] = 1.9f;
			config.SceneSliderRotations[3] = 0.0f;
			config.SceneSliderRotations[4] = 0.0f;
			config.SceneSliderRotations[5] = 0.0f;
		}
		if (ImGui::Button("Preset 3"))
		{
			config.SceneSliderRotations[0] = 0.785f;
			config.SceneSliderRotations[1] = 0.785f;
			config.SceneSliderRotations[2] = 1.9f;
			config.SceneSliderRotations[3] = 0.0f;
			config.SceneSliderRotations[4] = 10.465f - 6.28f;
			config.SceneSliderRotations[5] = 0.0f;
		}
		if (ImGui::Button("Preset 4"))
		{
			config.SceneSliderRotations[0] = 0.785f;
			config.SceneSliderRotations[1] = 0.785f;
			config.SceneSliderRotations[2] = 2.465f;
			config.SceneSliderRotations[3] = 1.495f;
			config.SceneSliderRotations[4] = 0.0f;
			config.SceneSliderRotations[5] = 0.0f;
		}
		if (ImGui::Button("Preset Screenshot"))
		{
			config.SceneSliderRotations[0] = 0.785f;
			config.SceneSliderRotations[1] = 6.900f - 6.28f;
			config.SceneSliderRotations[2] = 4.150f;
			config.SceneSliderRotations[3] = 0.0f;
			config.SceneSliderRotations[4] = 0.0f;
			config.SceneSliderRotations[5] = 0.0f;
		}
		if (ImGui::Button("Preset Screenshot B"))
		{
			config.SceneSliderRotations[0] = 0.785f;
			config.SceneSliderRotations[1] = 6.900f - 6.28f;
			config.SceneSliderRotations[2] = 4.150f;
			config.SceneSliderRotations[3] = -0.040f + 6.28f;
			config.SceneSliderRotations[4] = 6.500f;
			config.SceneSliderRotations[5] = 0.0f;
		}

		ImGui::Text("Scene Object Position Sliders");
		
		ImGui::SliderFloat("Pos X",	&config.SceneSliderPositions[0], -200.0f, 200.0f);
		ImGui::SliderFloat("Pos Y",	&config.SceneSliderPositions[1], -200.0f, 200.0f);
		ImGui::SliderFloat("Pos Z",	&config.SceneSliderPositions[2], -200.0f, 200.0f);
		ImGui::SliderFloat("Pos W",	&config.SceneSliderPositions[3], -200.0f, 200.0f);

		if (ImGui::Button("Reset Position"))
		{
			for (int i = 0; i < 4; i++) 
			{
				config.SceneSliderPositions[i] = 0.0f;
			}
		}
			ImGui::EndTabItem();
		}
		
		if (ImGui::BeginTabItem("Camera"))
		{
			ImGui::Text("Camera Position: %.2f, %.2f, %.2f, %.2f", application.m_LastCameraPosition.x, application.m_LastCameraPosition.y, application.m_LastCameraPosition.z, application.m_LastCameraPosition.w);
			ImGui::Text("Camera Latitude 4D: %.2f deg", glm::degrees(application.m_CameraAngleZW));
			ImGui::Text("Camera Latitude 3D: %.2f deg", glm::degrees(application.m_CameraAngleYZ));
			ImGui::Text("Camera Longitude: %.2f deg", glm::degrees(application.m_CameraAngleXY));
			ImGui::Text("Camera Right: %.2f, %.2f, %.2f, %.2f", application.m_LastCameraRight.x, application.m_LastCameraRight.y, application.m_LastCameraRight.z, application.m_LastCameraRight.w);
			ImGui::Text("Camera Up: %.2f, %.2f, %.2f, %.2f", application.m_LastCameraUp.x, application.m_LastCameraUp.y, application.m_LastCameraUp.z, application.m_LastCameraUp.w);
			ImGui::Text("Camera Forward: %.2f, %.2f, %.2f, %.2f", application.m_LastCameraForward.x, application.m_LastCameraForward.y, application.m_LastCameraForward.z, application.m_LastCameraForward.w);
			ImGui::Text("Camera Over: %.2f, %.2f, %.2f, %.2f", application.m_LastCameraOver.x, application.m_LastCameraOver.y, application.m_LastCameraOver.z, application.m_LastCameraOver.w);
		
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::End();

	m_FirstTick = false;
	//////////////////////////////////////////////////////////////////////////

}