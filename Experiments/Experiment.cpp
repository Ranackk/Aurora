#include "stdafx.h"
#include "Experiment.h"
#include "Vendor/imgui/imgui_stdlib.h"
#include "ColorSchemes.h"
#include <Rendering/Application.h>
#include <Options/Configuration.h>
#include <map>

void OppositePrimtiveExperiment::Prepare(Application& application, Configuration& configuration)
{
	constexpr unsigned short TEST_CATEGORIES	= 6;
	constexpr unsigned short TEST_PER_CATEGORY	= 3;
	constexpr unsigned short TRIALS				= 1;
	// Experiment Config
	unsigned int seed = 0;
	
	DataForTests.resize(TEST_CATEGORIES * TEST_PER_CATEGORY + TRIALS);
	std::vector<OppositePrimitveTestData> dataToBeSchuffled;
	dataToBeSchuffled.reserve((TEST_CATEGORIES - 1) * TEST_PER_CATEGORY);

	constexpr unsigned short BASE		= static_cast<unsigned short>(0);
	constexpr unsigned short THREE_DIM	= static_cast<unsigned short>(DerivationFlags::AutostereoscopicView);
	constexpr unsigned short TEXTURE	= static_cast<unsigned short>(DerivationFlags::Texture);
	constexpr unsigned short SHADING	= static_cast<unsigned short>(DerivationFlags::Shading);
	
	// Base
	DataForTests[0].Setup(BASE, true, ++seed, 0);	// < trial
	DataForTests[1].Setup(BASE, false, ++seed, 2);
	DataForTests[2].Setup(BASE, false, ++seed, 4);
	DataForTests[3].Setup(BASE, false, ++seed, 6);

	// Derivations
	// Base + 3D
	dataToBeSchuffled.push_back(OppositePrimitveTestData(BASE | THREE_DIM, false, ++seed, 4));
	dataToBeSchuffled.push_back(OppositePrimitveTestData(BASE | THREE_DIM, false, ++seed, 0));
	dataToBeSchuffled.push_back(OppositePrimitveTestData(BASE | THREE_DIM, false, ++seed, 6));
	
	// Base + Shading
	dataToBeSchuffled.push_back(OppositePrimitveTestData(BASE | SHADING, false, ++seed, 4));
	dataToBeSchuffled.push_back(OppositePrimitveTestData(BASE | SHADING, false, ++seed, 0));
	dataToBeSchuffled.push_back(OppositePrimitveTestData(BASE | SHADING, false, ++seed, 6));

	// Base + Texture
	dataToBeSchuffled.push_back(OppositePrimitveTestData(BASE | TEXTURE, false, ++seed, 4));
	dataToBeSchuffled.push_back(OppositePrimitveTestData(BASE | TEXTURE, false, ++seed, 0));
	dataToBeSchuffled.push_back(OppositePrimitveTestData(BASE | TEXTURE, false, ++seed, 6));

	// Base + Shading + Texture
	dataToBeSchuffled.push_back(OppositePrimitveTestData(BASE | TEXTURE | SHADING, false, ++seed, 4));
	dataToBeSchuffled.push_back(OppositePrimitveTestData(BASE | TEXTURE | SHADING, false, ++seed, 0));
	dataToBeSchuffled.push_back(OppositePrimitveTestData(BASE | TEXTURE | SHADING, false, ++seed, 6));

	// Base + 3D + Shading + Texture
	dataToBeSchuffled.push_back(OppositePrimitveTestData(BASE | THREE_DIM | TEXTURE | SHADING, false, ++seed, 4));
	dataToBeSchuffled.push_back(OppositePrimitveTestData(BASE | THREE_DIM | TEXTURE | SHADING, false, ++seed, 0));
	dataToBeSchuffled.push_back(OppositePrimitveTestData(BASE | THREE_DIM | TEXTURE | SHADING, false, ++seed, 6));

	constexpr unsigned short orderSeed = 42;
	std::default_random_engine generator(orderSeed);
	std::shuffle(dataToBeSchuffled.begin(), dataToBeSchuffled.end(), generator);

	unsigned short index = 4;
	for (OppositePrimitveTestData& data : dataToBeSchuffled)
	{
		DataForTests[index] = data;
		index ++;
	}

	//////////////////////////////////////////////////////////////////////////

	CountTrialTests		= 0;
	CountNonTrialTests	= 0;
	for (const auto& dataPoint : DataForTests)
	{
		if (dataPoint.IsTrial)
		{
			CountTrialTests ++;
		}
		else
		{
			CountNonTrialTests ++;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Switch to test

	BeginTest(application, configuration, 0);
}

//////////////////////////////////////////////////////////////////////////

ExperimentStatus OppositePrimtiveExperiment::DrawTaskUI(Application& application, Configuration& configuration)
{
	OppositePrimitveTestData& currentData = DataForTests[CurrentTestID];

	// Draw UI.

	if (currentData.IsTrial)
	{
		ImGui::Text("Trial Round %i/%i", (CurrentTestID + 1), CountTrialTests);
	}
	else
	{
		ImGui::Text("Round %i/%i", (CurrentTestID - CountTrialTests + 1), CountNonTrialTests);
	}
	ImGui::Separator();

	//////////////////////////////////////////////////////////////////////////

	int oppositeColorID				= currentData.Scheme.ColorIDs[currentData.TargetIsOppositeOfPrimitiveID_Plain];
	ResultColor oppositeColor		= Colors::GetColorByID(oppositeColorID);
	std::string oppositeColorName	= Colors::s_CubeColorNames[oppositeColorID];
	
	ImGui::Text("Task:");
	ImGui::Text("Name the cube-primitive that is opposite");
	ImGui::Text("to the cube-primitive colored");
	ImGui::SameLine();
	ImGui::TextColored(oppositeColor, "%hs", oppositeColorName.c_str());
	ImGui::Separator();

	//////////////////////////////////////////////////////////////////////////

	for (int buttonID = 0; buttonID < Colors::PRIMITVE_COLOR_COUNT; buttonID++)
	{
		int primitiveID_Plain	= Colors::ColorScheme::GetColorIDForButtonID(CurrentTestID, buttonID);
		
		if (primitiveID_Plain == currentData.TargetIsOppositeOfPrimitiveID_Plain)
		{
			// Skip the primitive the question is asked for.
			continue;

			// Hard to let out when looking for +z: -y;
		}
		
		int colorID				= currentData.Scheme.ColorIDs[primitiveID_Plain];
		ResultColor color		= Colors::GetColorByID(colorID);
		std::string colorName	= Colors::s_CubeColorNames[colorID];
			
		//if (buttonID != 0 && buttonID != 4)	ImGui::SameLine();

		bool buttonPreessed = ImGui::ColorButton(colorName.c_str(), color, ImGuiColorEditFlags_NoTooltip);
		ImGui::SameLine();
		ImGui::Text(colorName.c_str());
		
		static bool CHEAT_MODE = false;
		static bool FEEDBACK_MODE = false;
		if (CHEAT_MODE)
		{
			bool isCorrectAnswer = currentData.TargetIsOppositeOfPrimitiveID_Plain == Colors::ColorScheme::GetOppositeID(primitiveID_Plain);
			if (isCorrectAnswer)
			{
				ImGui::SameLine();
				ImGui::Text("<- this one");
			}
		}

		if (buttonPreessed)
		{
			currentData.SelectedPrimitveID_Plain	= primitiveID_Plain;
			currentData.TimeMS						= static_cast<unsigned long>(application.GetTimeSinceStartupMyS().count() / 1000) - TimeBeginExperiment;
			
			if (FEEDBACK_MODE)
			{
				bool isCorrectAnswer = currentData.TargetIsOppositeOfPrimitiveID_Plain == Colors::ColorScheme::GetOppositeID(primitiveID_Plain);
				printf("That was %hs", isCorrectAnswer ? "CORRECT!" : "Wrong my dude.");
			}

			if (CurrentTestID < DataForTests.size() - 1)
			{
				BeginTest(application, configuration, CurrentTestID + 1);
				return ExperimentStatus::InExperiment;
			}
			else
			{
				return ExperimentStatus::ExperimentDone;
			}
		}
	}

	return ExperimentStatus::InExperiment;
}

//////////////////////////////////////////////////////////////////////////

void OppositePrimtiveExperiment::BeginTest(Application& application, Configuration& configuration, int id)
{
	CurrentTestID								= id;
	OppositePrimitveTestData& currentData		= DataForTests[CurrentTestID];
	TimeBeginExperiment							= static_cast<unsigned long>(application.GetTimeSinceStartupMyS().count() / 1000);

	configuration.ActiveColorScheme				= currentData.Scheme;
	configuration.DrawModeHit					= GetDrawMode(currentData.Derivation);
	configuration.DrawModeMiss					= configuration.DrawModeHit;
	
	bool autoStereoscopic						= CheckDerivationFlag(currentData.Derivation, DerivationFlags::AutostereoscopicView);
	application.ActiveQuiltConfiguration		= autoStereoscopic ? QuiltConfiguration::_2k_4x8 : QuiltConfiguration::_2k_1x1;
	application.DrawQuiltInsteadOfLightfield	= false;

	// Reset View
	for (int i = 0; i < 6; i++)
	{
		configuration.SceneSliderRotations[i] = configuration.STUDY_PERSPECTIVE[i];
	}
	
}

//////////////////////////////////////////////////////////////////////////

void OppositePrimtiveExperiment::PrintResults(std::ofstream& inOutFile)
{
	struct ResultData
	{
		unsigned short correctCountedAnswers = 0;
		unsigned short countedAnswers = 0;
		unsigned long  totalCountedAnswerTime = 0;
	};

	auto printResultData = [&inOutFile] (ResultData& data)
	{
		inOutFile << "Correct answers: " << data.correctCountedAnswers << "/" << data.countedAnswers << "(" << (data.correctCountedAnswers / static_cast<float>(data.countedAnswers) * 100.0f) << "%)" << std::endl;	
		inOutFile << "Time: " << data.totalCountedAnswerTime << " (per answer " << (data.totalCountedAnswerTime / static_cast<float>(data.countedAnswers)) << ")" << std::endl;	
	};

	auto transformAxisNumber = [] (unsigned short axisNumber)
	{
		switch (axisNumber)
		{
			case 0: return "+x";
			case 1: return "-x";
			case 2: return "+y";
			case 3: return "-y";
			case 4: return "+z";
			case 5: return "-z";
			case 6: return "+w";
			case 7: return "-w";
		}

		return "WRONG AXIS";
	};
	
	inOutFile << "\"Opposite Primitives\"" << std::endl;
	
	//////////////////////////////////////////////////////////////////////////
	// Raw

	inOutFile << "Raw Results:" << std::endl;

	// Data
	unsigned short testID = 0;
	ResultData generalData;
	std::array<ResultData, static_cast<unsigned short>(DerivationFlags::Count)> derviationFlagData;
	std::map<unsigned int, ResultData> uniqueDerivationData;

	for (OppositePrimitveTestData& testData : DataForTests)
	{
		bool autoSteorgrahpicView			= CheckDerivationFlag(testData.Derivation, DerivationFlags::AutostereoscopicView);
		bool shading						= CheckDerivationFlag(testData.Derivation, DerivationFlags::Shading);
		bool texture						= CheckDerivationFlag(testData.Derivation, DerivationFlags::Texture);
		
		unsigned int expectedIDLocal		= Colors::ColorScheme::GetOppositeID(testData.TargetIsOppositeOfPrimitiveID_Plain);
		bool isCorrect						= testData.SelectedPrimitveID_Plain == expectedIDLocal;

		unsigned int givenColorIDGlobal		= testData.Scheme.ColorIDs[testData.TargetIsOppositeOfPrimitiveID_Plain];
		unsigned int selectedIDGlobal		= testData.Scheme.ColorIDs[testData.SelectedPrimitveID_Plain];
		unsigned int expectedIDGlobal		= testData.Scheme.ColorIDs[expectedIDLocal];

		const char* givenPrimtiveAxis		= transformAxisNumber(testData.TargetIsOppositeOfPrimitiveID_Plain);
		const char* selectedPrimtiveAxis	= transformAxisNumber(testData.SelectedPrimitveID_Plain);
		const char* expectedPrimtiveAxis	= transformAxisNumber(expectedIDLocal);
		
		const char* givenPrimitveColor		= Colors::s_CubeColorNames[givenColorIDGlobal];
		const char* selectedPrimitveColor	= Colors::s_CubeColorNames[selectedIDGlobal];
		const char* expectedPrimitveColor	= Colors::s_CubeColorNames[expectedIDGlobal];

		if (!testData.IsTrial)
		{
			// General
			generalData.countedAnswers ++;
			generalData.totalCountedAnswerTime += testData.TimeMS;
			generalData.correctCountedAnswers += static_cast<unsigned int> (isCorrect);

			// By Flag
			for (int i = 0; i < static_cast<unsigned short>(DerivationFlags::Count); i++)
			{
				DerivationFlags flag = static_cast<DerivationFlags> (1 << i);
				if (CheckDerivationFlag(testData.Derivation, flag))
				{
					derviationFlagData[i].countedAnswers ++;
					derviationFlagData[i].totalCountedAnswerTime += testData.TimeMS;
					derviationFlagData[i].correctCountedAnswers += static_cast<unsigned int> (isCorrect);
				}
			}

			// by UniqueDerivation
			//bool existingKey = uniqueDerivationData.find(testData.Derivation) != uniqueDerivationData.end();
			uniqueDerivationData[testData.Derivation].countedAnswers ++;
			uniqueDerivationData[testData.Derivation].totalCountedAnswerTime += testData.TimeMS;
			uniqueDerivationData[testData.Derivation].correctCountedAnswers += static_cast<unsigned int> (isCorrect);
		}


		//////////////////////////////////////////////////////////////////////////
		// Print

		testID ++;
		inOutFile << "Test " << testID << "/" << DataForTests.size() << (testData.IsTrial ? "[TRIAL ROUND]" : "") << std::endl;
		inOutFile << "Derivation: 3D = " << (autoSteorgrahpicView ? "ON" : "OFF") << ", Shading = " << (shading ? "ON" : "OFF") << ", Texture = " << (texture ? "ON" : "OFF") << std::endl;
		inOutFile << "Asked: Primitive opposite of " << givenPrimtiveAxis << "(" << givenPrimitveColor << ")" << std::endl;
		inOutFile << "Expected: Primitive " << expectedPrimtiveAxis << "(" << expectedPrimitveColor << ")" << std::endl;
		inOutFile << "Selected: Primitive " << selectedPrimtiveAxis << "(" << selectedPrimitveColor << ")" << std::endl;
		inOutFile << "Answered in " << testData.TimeMS << " [ms]" << std::endl;
		inOutFile << "Result: " << (isCorrect ? "CORRECT" : "WRONG") << std::endl;
		inOutFile << std::endl;
	}

	inOutFile << std::endl;
	inOutFile << std::endl;

	//////////////////////////////////////////////////////////////////////////
	// Statistics
	
	constexpr unsigned short keyBaseData = static_cast<unsigned short>(DerivationFlags::Base);
	inOutFile << "Statistics:" << std::endl << std::endl;
	
	inOutFile << "General:" << std::endl;

	printResultData(generalData);
	inOutFile << std::endl;

	inOutFile << "By Unique Derivations:" << std::endl;
	for (auto kvp : uniqueDerivationData)
	{
		bool autoSteorgrahpicView = CheckDerivationFlag(kvp.first, DerivationFlags::AutostereoscopicView);
		bool shading			  = CheckDerivationFlag(kvp.first, DerivationFlags::Shading);
		bool texture			  = CheckDerivationFlag(kvp.first, DerivationFlags::Texture);

		inOutFile << "Combination " << kvp.first << ": Base ";
		if (autoSteorgrahpicView) inOutFile << "+ 3D ";
		if (shading) inOutFile << "+ Shading ";
		if (texture) inOutFile << "+ Texture ";
		inOutFile << std::endl;

		printResultData(kvp.second);
		
		// VS. Base

		if (kvp.first == keyBaseData)
		{
			inOutFile << std::endl;
			continue;
		}

		float correctPercentageBase = uniqueDerivationData[keyBaseData].correctCountedAnswers / static_cast<float>(uniqueDerivationData[keyBaseData].countedAnswers);
		float correctPercentageThis = kvp.second.correctCountedAnswers / static_cast<float>(kvp.second.countedAnswers);
		
		float averageTimeBase		= uniqueDerivationData[keyBaseData].totalCountedAnswerTime / static_cast<float>(uniqueDerivationData[keyBaseData].countedAnswers);
		float averageTimeThis		= kvp.second.totalCountedAnswerTime / static_cast<float>(kvp.second.countedAnswers);

		inOutFile << "VS BASE: " << std::endl;
		inOutFile << "Correct %: ";
		inOutFile << ((correctPercentageThis >= correctPercentageBase) ? "+" : "");
		inOutFile << ((correctPercentageThis - correctPercentageBase) * 100.0f) << "%" << std::endl;
		inOutFile << "Average Time: ";
		inOutFile << ((averageTimeThis >= averageTimeBase) ? "+" : "-");
		inOutFile << std::abs(averageTimeThis - averageTimeBase) << " ms" << std::endl;
		inOutFile << std::endl;


	}
	inOutFile << std::endl;
	
	inOutFile << "By Derivations Flags:" << std::endl;
	
	for (int i = 0; i < static_cast<unsigned short>(DerivationFlags::Count); i++)
	{
		inOutFile << s_DerivationFlagNames[i] << ":" << std::endl;		
		printResultData(derviationFlagData[i]);
		inOutFile << std::endl;
	}
	inOutFile << std::endl;
	inOutFile << std::endl;
}