// Copyright Epic Games, Inc. All Rights Reserved.

#include "Data/StudyProblemDataAsset.h"

UStudyProblemDataAsset::UStudyProblemDataAsset()
	: CorrectAnswerExperience{100}
{
}

const FString& UStudyProblemDataAsset::GetProblemId() const
{
	return ProblemId;
}

FName UStudyProblemDataAsset::GetSubjectTopicName() const
{
	return SubjectTopicName;
}

FName UStudyProblemDataAsset::GetSectionTopicName() const
{
	return SectionTopicName;
}

int32 UStudyProblemDataAsset::GetCorrectAnswerExperience() const
{
	return CorrectAnswerExperience;
}

FName UStudyProblemDataAsset::GetUnlockedSectionName() const
{
	return UnlockedSectionName;
}
