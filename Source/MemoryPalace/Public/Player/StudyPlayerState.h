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

	/* Fires whenever a topic gains experience */
	FStudyExperienceChanged OnExperienceChanged;

private:
	/* When each part of each problem last earned experience */
	TMap<FString, FDateTime> LastAwardTimeByPartKey;

	/* Experience earned so far, by topic kind then topic name */
	TMap<EStudyTopicKind, TMap<FName, int32>> ExperienceByKind;
};
