// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "StudyPlayerState.generated.h"

/* Fires with a topic and its new experience total; only the HUD
   listens, so it is not multicast */
DECLARE_DELEGATE_TwoParams(FStudyExperienceChanged, FName, int32);

/* Holds the experience the student has earned in each topic */
UCLASS()
class MEMORYPALACE_API AStudyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	/* Sets default values for this player state */
	AStudyPlayerState();

	/* Adds experience to a topic and announces the new total */
	void AddExperience(FName TopicName, int32 ExperienceAmount);

	/* Returns the experience earned in a topic, or zero */
	int32 GetExperience(FName TopicName) const;

	/* Fires whenever a topic gains experience */
	FStudyExperienceChanged OnExperienceChanged;

private:
	/* Experience earned so far, keyed by topic name */
	TMap<FName, int32> ExperienceByTopic;
};
