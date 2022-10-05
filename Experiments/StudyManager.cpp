#include "stdafx.h"
#include "StudyManager.h"
#include <time.h>
#include <iomanip>
#include <iostream>
#include <iosfwd>
#include <sstream> 
#include <direct.h>
#include <fstream>
#include <cstdlib>
#include <filesystem>

#include <Rendering/Application.h>
#include <Vendor/imgui/imgui_stdlib.h>

bool StudyManager::IsParticipantDataValid() const
{	
	if (m_ParticipantName.size() <= 2) return false;
	if (m_ParticipantAge < 12 || m_ParticipantAge > 112) return false;
	if (m_ParticipantGender == Gender::Count) return false;
	if (m_ParticipantBackground == AcademicBackground::Count) return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////

void StudyManager::StartExperiment(Application& application, Configuration& configuration, const unsigned short experimentID)
{
	if (experimentID >= EXPERIMENTS.size())
	{
		return;
	}

	m_CurrentExperimentID = experimentID;

	const ExperimentType experimentType = EXPERIMENTS[m_CurrentExperimentID];
	Experiment* const experiment = CreateExperiment(experimentType);
	experiment->Prepare(application, configuration);
	
	m_Experiments.push_back(experiment);
}

//////////////////////////////////////////////////////////////////////////

void StudyManager::Cleanup()
{
	for (int i = 0; i < m_Experiments.size(); i++)
	{
		delete m_Experiments[i];
	}
	m_Experiments.clear();
}

//////////////////////////////////////////////////////////////////////////

void StudyManager::BeginStudy()
{
	printf("BeginStudy\n");
}

//////////////////////////////////////////////////////////////////////////

void StudyManager::FinishStudy()
{
	std::stringstream fileStream;
	std::tm timeinfo;
	auto startupTime = Application::Instance()->GetApplicationStartupTime(); 
	localtime_s(&timeinfo, &startupTime);

	std::filesystem::create_directories("../Results/Study");

	fileStream << "../Results/Study/" << m_ParticipantName << "_" << std::put_time(&timeinfo, "%m-%d_%H-%M") << ".txt";
	
	//////////////////////////////////////////////////////////////////////////

	const std::string filePath = fileStream.str();

	//////////////////////////////////////////////////////////////////////////
	// Write to file
	
	std::ofstream file(filePath);
	if (!file.is_open())
	{
		return;
	}
	
	file << "STUDY RESULTS" << std::endl;
	file << "Name:" << m_ParticipantName.c_str() << std::endl;
	file << "Age:" << m_ParticipantAge << std::endl;
	file << "Gender:" << s_GenderNames[static_cast<int>(m_ParticipantGender)] << std::endl;
	file << "Academic Background:" << s_AcademicBackgroundNames[static_cast<int>(m_ParticipantBackground)] << std::endl;
	
	file << std::endl;

	unsigned short experimentID = 0;
	for (Experiment* const experiment : m_Experiments)
	{
		experimentID ++;
		file << "EXPERIMENT " << experimentID << std::endl;
		experiment->PrintResults(file);
		file << std::endl;
		file << std::endl;
	}


	file.close();
	printf("FinishedStudy\n");

	Cleanup();
}

//////////////////////////////////////////////////////////////////////////

void StudyManager::Update(Application& application, Configuration& configuration)
{
	const ImVec2 size		= ImVec2(600, 500);
	glm::ivec2 windowPos	= application.GetWindowPosition() - glm::ivec2(size.x, size.y) + glm::ivec2(application.GetWindowSize().x / 2.0f + size.x, 0.0f);
	windowPos.x				= glm::max(0, windowPos.x);
	windowPos.y				= glm::max(0, windowPos.y);
	const ImVec2 position1	= ImVec2(static_cast<float>(windowPos.x), static_cast<float>(windowPos.y));
	const ImVec2 position2	= ImVec2(static_cast<float>(windowPos.x + size.x), static_cast<float>(windowPos.y));

	ImGui::SetNextWindowPos(position1);
	ImGui::SetNextWindowSize(size);

	ImGui::Begin("Experiment Task");
	DrawExperimentTaskUI(application, configuration);
	ImGui::End();

	if (m_State == StudyState::LearningMode || m_State == StudyState::TestMode)
	{
		ImGui::SetNextWindowPos(position2);
		ImGui::SetNextWindowSize(size);

		ImGui::Begin("Experiment Control");
		DrawExperimentControlUI(application, configuration);
		ImGui::End();
	}
}

//////////////////////////////////////////////////////////////////////////

void StudyManager::DrawExperimentTaskUI(Application& application, Configuration& configuration)
{
	switch (m_State)
	{
		case StudyState::FillInParticipantData:
		{
			ImGui::Text("Welcome to the experiment! \nPlease fill in the following data:");

			// Name
			ImGui::InputText("Name", &m_ParticipantName);
			
			// Age
			int age = static_cast<int>(m_ParticipantAge);
			ImGui::InputInt("Age", &age);
			m_ParticipantAge = (unsigned short) age;
			
			// Gender
			int genderID = static_cast<int>(m_ParticipantGender);
			ImGui::Combo("Gender", &genderID, s_GenderNames, static_cast<int>(Gender::Count));
			m_ParticipantGender = static_cast<Gender>(genderID);

			// Academic Background
			int academicID = static_cast<int>(m_ParticipantBackground);
			ImGui::Combo("Academic Background", &academicID, s_AcademicBackgroundNames, static_cast<int>(AcademicBackground::Count));
			m_ParticipantBackground = static_cast<AcademicBackground>(academicID);

			if (IsParticipantDataValid())
			{
				if (ImGui::Button("Submit"))
				{
					// Switch to trial mode
					// Set experiment to first experiment#
					BeginStudy();
					m_State = StudyState::LearningMode;
					return;
				}
			}

			break;
		} 

		case StudyState::LearningMode:
		{
			ImGui::Text("Learning Mode");
			ImGui::Text("Make yourself comfortable with the");
			ImGui::Text("controls shown in the controls box.");
			ImGui::Text("When you are finished and feel like");
			ImGui::Text("you got a rough understanding of");
			ImGui::Text("what you are doing, notify Jan");
			ImGui::Text("and start the experiment.");

			if (ImGui::Button("Start Experiment"))
			{
				// Switch to test mode
				StartExperiment(application, configuration, m_CurrentExperimentID);
				m_State = StudyState::TestMode;
				return;
			}
			break;
		}

		case StudyState::TestMode:
		{
			const ExperimentStatus status = m_Experiments[m_CurrentExperimentID]->DrawTaskUI(application, configuration);
			if (status == ExperimentStatus::ExperimentDone)
			{
				const bool wasLastExperiment = m_CurrentExperimentID == EXPERIMENTS.size() - 1;
				if (wasLastExperiment)
				{
					FinishStudy();
					m_State = StudyState::Done; 
					return;
				}
				else
				{
					StartExperiment(application, configuration, m_CurrentExperimentID + 1);
					return;
				}
			}
			break;
		} 

		case StudyState::Done:
		{
			ImGui::Text("Thank you for finishing the study!");
			if (ImGui::Button("Retake"))
			{
				BeginStudy();
				StartExperiment(application, configuration, 0);
				m_State = StudyState::LearningMode;
				return;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void StudyManager::DrawExperimentControlUI(Application& application, Configuration& configuration)
{
	const auto drawSlider = [this, &configuration] (const std::string& label, const unsigned int id)
	{
		ImGui::PushID(id);
		ImGui::SliderFloat(label.c_str(), &(configuration.SceneSliderRotations[id]), -(1.2f * (id % 3) + 1.7f) * glm::pi<float>(), (1.36f * (id % 3) + 2.0f) * glm::pi<float>(), "");
		ImGui::SameLine();
		if (ImGui::Button("Reset"))
		{
			configuration.SceneSliderRotations[id] = configuration.STUDY_PERSPECTIVE[id];
			StartRotationTimeMS = -1;
		}
		ImGui::PopID();
	};

	const auto drawPresetButton = [this, &application, &configuration] (const unsigned int id, const std::string& identifier, const float rotation0, const float rotation1, const float rotation2, const float rotation3, const float rotation4, const float rotation5)
	{
		ImGui::PushID(id);
		if (ImGui::Button(identifier.c_str()))
		{
			for (int i = 0; i < 6; i++)
			{
				BeginRotation[i] = configuration.SceneSliderRotations[i];
			}

			TargetRotation[0]	= rotation0;
			TargetRotation[1]	= rotation1;
			TargetRotation[2]	= rotation2;
			TargetRotation[3]	= rotation3;
			TargetRotation[4]	= rotation4;
			TargetRotation[5]	= rotation5;

			StartRotationTimeMS = static_cast<unsigned long>(application.GetTimeSinceStartupMyS().count() / 1000);
		}
		ImGui::PopID();
	};

	if (StartRotationTimeMS != -1)
	{
		constexpr unsigned long ROTATION_TIME_MS = 800;

		const float animationTime = static_cast<float>((application.GetTimeSinceStartupMyS().count() / 1000) - StartRotationTimeMS);
		const float lerpFactor	= Math::Clamp01(animationTime / ROTATION_TIME_MS);

		for (int i = 0; i < 6; i++)
		{
			configuration.SceneSliderRotations[i] = glm::lerp(BeginRotation[i], TargetRotation[i], lerpFactor);
		}

		// Finish
		if (lerpFactor == 1.0f)
		{
			StartRotationTimeMS = -1;
		}
	}

	switch (m_State)
	{
		case StudyState::FillInParticipantData:
		case StudyState::Done:
			return;

		case StudyState::LearningMode:
		case StudyState::TestMode:
		{
			ImGui::Text("Rotate Via Sliders");
			drawSlider("1", 0);  // Rot ZW (z-Axis)
			drawSlider("2", 1);  // Rot YW (-y-Axis)
			drawSlider("3", 3);  // Rot XW (x-Axis)
			ImGui::Text(" ");
			drawSlider("4", 2);  // Rot YZ 		
			drawSlider("5", 4);  // Rot XZ 		
			drawSlider("6", 5);  // Rot XY	
			if (ImGui::Button("Reset All"))
			{
				for (int i = 0; i < 6; i++)
				{
					configuration.SceneSliderRotations[i] = configuration.STUDY_PERSPECTIVE[i];
				}
				StartRotationTimeMS = -1;
			}

			ImGui::Separator();	
			
			ImGui::Text("Rotate Via Presets");

			// A:    | +y | -z | +w
			// B:    | +y | -z | -w
			// C: +x | -y | -z | 
			// D:    | -y | -z | +w
			// E:    | +y | +z | -w

			ImGui::Text("Presets:");
			drawPresetButton(8, "Default", configuration.STUDY_PERSPECTIVE[0], configuration.STUDY_PERSPECTIVE[1], configuration.STUDY_PERSPECTIVE[2], configuration.STUDY_PERSPECTIVE[3], configuration.STUDY_PERSPECTIVE[4], configuration.STUDY_PERSPECTIVE[5]);
			ImGui::SameLine(); 
			drawPresetButton(8, "A", 2.303f, 2.222f, 7.314f - 6.28f, 3.596f, 0.0f, 0.0f);
			ImGui::SameLine(); 
			drawPresetButton(9, "B", 2.303f, 2.222f, 8.445f - 6.28f, 3.596f, 0.0f, 0.0f);
			ImGui::SameLine(); 
			drawPresetButton(10, "C", 2.303f, 2.222f, 4.0f, 3.596f, 0.0f, 0.0f);
			ImGui::SameLine();
			drawPresetButton(11, "D", 2.303f, 2.222f, 4.0f, 3.596f, 2.303f, 0.0f);
			ImGui::SameLine();
			drawPresetButton(12, "E", 2.303f, 6.667f - 6.28f, 4.0f, 3.596f, 0.0f, 0.0f);
			ImGui::SameLine();
			drawPresetButton(13, "F", -0.768f + 6.28f, 0.387f, 4.0f, 3.596f, 0.0f, 0.0f);
		} 

	}
}

