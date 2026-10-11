// Copyright Epic Games, Inc. All Rights Reserved.

#include "Data/StudyProgressSaveGame.h"

FStudySavedPart::FStudySavedPart()
	: bIsCorrect{false}
{
}

FStudySavedSubmission::FStudySavedSubmission()
	: CorrectPartCount{0}
	, PartCount{0}
	, ExperienceGained{0}
	, ElapsedSeconds{0.0}
	, bIsCorrect{false}
{
}

FStudySavedProblem::FStudySavedProblem()
	: bIsSolved{false}
{
}
