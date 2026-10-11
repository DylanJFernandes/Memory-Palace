// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/StudyPlayerState.h"

AStudyPlayerState::AStudyPlayerState()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AStudyPlayerState::AddExperience(
	EStudyTopicKind TopicKind, FName TopicName,
	int32 ExperienceAmount)
{
	TMap<FName, int32>& TopicExperience{
		ExperienceByKind.FindOrAdd(TopicKind)};
	int32& TotalExperience{TopicExperience.FindOrAdd(TopicName)};
	TotalExperience += ExperienceAmount;
	OnExperienceChanged.ExecuteIfBound(
		TopicKind, TopicName, TotalExperience);
}

int32 AStudyPlayerState::GetExperience(
	EStudyTopicKind TopicKind, FName TopicName) const
{
	const TMap<FName, int32>* TopicExperience{
		ExperienceByKind.Find(TopicKind)};
	if (!TopicExperience)
	{
		return 0;
	}
	const int32* TotalExperience{TopicExperience->Find(TopicName)};
	if (!TotalExperience)
	{
		return 0;
	}
	return *TotalExperience;
}
