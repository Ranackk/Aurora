#pragma once
#include "Experiment.h"
#include "ExperimentTypes.h"
#include <string>

class StudyManager
{
	enum class StudyState
	{
		FillInParticipantData	= 0,
		LearningMode			= 1,
		TestMode				= 2,
		Done					= 3
	};

	const std::vector<ExperimentType> EXPERIMENTS = {ExperimentType::BorderingPrimitves};

	StudyState m_State = StudyState::FillInParticipantData;
	unsigned short m_CurrentExperimentID = 0;
	std::vector<Experiment*> m_Experiments;

	// Participant Data
	std::string			m_ParticipantName = "Jon Snow";
	unsigned short		m_ParticipantAge = 18;
	Gender				m_ParticipantGender = Gender::Male;
	AcademicBackground	m_ParticipantBackground = AcademicBackground::Science;

	//////////////////////////////////////////////////////////////////////////

	// Perspective
	
	float				TargetRotation[6]	= {0.785f, 6.900f, 4.150f, -0.040f, 6.5f, 0.0f}; 
	float				BeginRotation[6]	= {0.785f, 6.900f, 4.150f, -0.040f, 6.5f, 0.0f}; 
	long				StartRotationTimeMS = -1;
	
	//////////////////////////////////////////////////////////////////////////

public:
	StudyManager() = default;
	void Update(Application& application, Configuration& configuration);

	//////////////////////////////////////////////////////////////////////////

private:
	bool IsParticipantDataValid() const;
	
	void DrawExperimentTaskUI(Application& application, Configuration& configuration);
	void DrawExperimentControlUI(Application& application, Configuration& configuration);
	
	void StartExperiment(Application& application, Configuration& configuration, const unsigned short experimentID);
	void BeginStudy();
	void FinishStudy();

	
	void Cleanup();
};

