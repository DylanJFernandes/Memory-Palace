// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/StudyPlayerState.h"

AStudyPlayerState::AStudyPlayerState()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AStudyPlayerState::AddExperience(
	FName TopicName, int32 ExperienceAmount)
{
	int32& TopicExperience{ExperienceByTopic.FindOrAdd(TopicName)};
	TopicExperience += ExperienceAmount;
	OnExperienceChanged.ExecuteIfBound(TopicName, TopicExperience);
}

int32 AStudyPlayerState::GetExperience(FName TopicName) const
{
	const int32* TopicExperience{ExperienceByTopic.Find(TopicName)};
	if (!TopicExperience)
	{
		return 0;
	}
	return *TopicExperience;
}
