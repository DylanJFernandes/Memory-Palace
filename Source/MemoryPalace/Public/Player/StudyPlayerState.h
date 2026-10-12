// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/StudyTopicKind.h"
#include "GameFramework/PlayerState.h"
#include "Misc/Optional.h"
#include "StudyPlayerState.generated.h"

/* Fires with a topic's kind, name and new experience total; only
   the HUD listens, so it is not multicast */
DECLARE_DELEGATE_ThreeParams(FStudyExperienceChanged,
							 EStudyTopicKind, FName, int32);

/* Holds the experience the student has earned in each topic */
UCLASS()
class MEMORYPALACE_API AStudyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	/* Sets default values for this player state */
	AStudyPlayerState();

	/* Adds experience to a topic and announces the new total */
	void AddExperience(EStudyTopicKind TopicKind, FName TopicName,
					   int32 ExperienceAmount);

	/* Returns the experience earned in a topic, or zero */
	int32 GetExperience(EStudyTopicKind TopicKind,
						FName TopicName) const;

	/* Remembers when a part of a problem last earned experience */
	void RecordPartAward(const FString& PartKey,
						 const FDateTime& AwardTime);

	/* Returns when a part last earned experience, if it ever did */
	TOptional<FDateTime> GetLastPartAwardTime(
		const FString& PartKey) const;

	/* Returns the experience earned in every topic of a kind */
	TMap<FName, int32> GetTopicExperience(
		EStudyTopicKind TopicKind) const;

	/* Returns when each part of each problem last earned
	   experience */
	const TMap<FString, FDateTime>& GetPartAwardTimes() const;

	/* Replaces the earned experience and award times with saved
	   ones, without announcing them */
	void RestoreProgress(
		const TMap<FName, int32>& SubjectExperience,
		const TMap<FName, int32>& SectionExperience,
		const TMap<FString, FDateTime>& PartAwardTimes);

	/* Announces the experience of every topic, so a listener that
	   bound late can catch up */
	void AnnounceAllExperience() const;

	/* Fires whenever a topic gains experience */
	FStudyExperienceChanged OnExperienceChanged;

private:
	/* When each part of each problem last earned experience */
	TMap<FString, FDateTime> LastAwardTimeByPartKey;

	/* Experience earned so far, by topic kind then topic name */
	TMap<EStudyTopicKind, TMap<FName, int32>> ExperienceByKind;
};
