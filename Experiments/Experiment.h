#pragma once
#include "ExperimentTypes.h"
#include "ColorSchemes.h"

class Application;


//////////////////////////////////////////////////////////////////////////

struct TestData
{
	// Setup
	unsigned short		Derivation = 0;
	bool				IsTrial	= false;
	Colors::ColorScheme Scheme;

	// Result
	unsigned long	TimeMS;

	//////////////////////////////////////////////////////////////////////////

	void Setup(unsigned short derivation, bool isTrial, unsigned int schemeSeed)
	{
		Derivation	= derivation;
		IsTrial		= isTrial;
		Scheme		= Colors::GenerateColorScheme(schemeSeed);
	}
};

struct OppositePrimitveTestData : TestData
{	
	// Setup
	unsigned short		TargetIsOppositeOfPrimitiveID_Plain = 0;

	// Result
	unsigned short		SelectedPrimitveID_Plain = 0;
	
	//////////////////////////////////////////////////////////////////////////

	OppositePrimitveTestData() = default;
	explicit OppositePrimitveTestData(const unsigned short derivation, const bool isTrial, const unsigned int schemeSeed, const unsigned short targetIsOppositeOfPrimitveID) : TargetIsOppositeOfPrimitiveID_Plain(targetIsOppositeOfPrimitveID)
	{
		TestData::Setup(derivation, isTrial, schemeSeed);
	}


	//////////////////////////////////////////////////////////////////////////

	void Setup(const unsigned short derivation, const bool isTrial, const unsigned int schemeSeed, const unsigned short targetIsOppositeOfPrimitveID)
	{
		TestData::Setup(derivation, isTrial, schemeSeed);

		TargetIsOppositeOfPrimitiveID_Plain = targetIsOppositeOfPrimitveID;
	}
};

//////////////////////////////////////////////////////////////////////////

struct Experiment
{
	virtual void Prepare(Application& application, Configuration& configuration) = 0;
	virtual ExperimentStatus DrawTaskUI(Application& application, Configuration& configuration) = 0;
	virtual void PrintResults(std::ofstream& inOutFile) = 0;
};

//////////////////////////////////////////////////////////////////////////

struct OppositePrimtiveExperiment : Experiment
{
	std::vector<OppositePrimitveTestData> DataForTests;
	unsigned int CurrentTestID			= 0;
	unsigned long TimeBeginExperiment	= 0l;

	// Visualisation
	unsigned short CountTrialTests;
	unsigned short CountNonTrialTests;
	
	void Prepare(Application& application, Configuration& configuration) override;
	ExperimentStatus DrawTaskUI(Application& application, Configuration& configuration) override;
	void PrintResults(std::ofstream& inOutFile) override;

	void BeginTest(Application& application, Configuration& configuration, const int id);
};