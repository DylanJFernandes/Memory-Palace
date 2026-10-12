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

void AStudyPlayerState::RecordPartAward(
	const FString& PartKey, const FDateTime& AwardTime)
{
	LastAwardTimeByPartKey.Add(PartKey, AwardTime);
}

TOptional<FDateTime> AStudyPlayerState::GetLastPartAwardTime(
	const FString& PartKey) const
{
	const FDateTime* AwardTime{LastAwardTimeByPartKey.Find(PartKey)};
	if (!AwardTime)
	{
		return TOptional<FDateTime>{};
	}
	return *AwardTime;
}

TMap<FName, int32> AStudyPlayerState::GetTopicExperience(
	EStudyTopicKind TopicKind) const
{
	const TMap<FName, int32>* TopicExperience{
		ExperienceByKind.Find(TopicKind)};
	if (!TopicExperience)
	{
		return TMap<FName, int32>{};
	}
	return *TopicExperience;
}

const TMap<FString, FDateTime>&
	AStudyPlayerState::GetPartAwardTimes() const
{
	return LastAwardTimeByPartKey;
}

void AStudyPlayerState::RestoreProgress(
	const TMap<FName, int32>& SubjectExperience,
	const TMap<FName, int32>& SectionExperience,
	const TMap<FString, FDateTime>& PartAwardTimes)
{
	ExperienceByKind.Add(EStudyTopicKind::Subject, SubjectExperience);
	ExperienceByKind.Add(EStudyTopicKind::Section, SectionExperience);
	LastAwardTimeByPartKey = PartAwardTimes;
}

void AStudyPlayerState::AnnounceAllExperience() const
{
	for (const TPair<EStudyTopicKind, TMap<FName, int32>>& Kind :
		 ExperienceByKind)
	{
		for (const TPair<FName, int32>& Topic : Kind.Value)
		{
			OnExperienceChanged.ExecuteIfBound(
				Kind.Key, Topic.Key, Topic.Value);
		}
	}
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
