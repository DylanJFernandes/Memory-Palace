// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "StudyGameState.generated.h"

/* Fires with the name of a section that just unlocked;
   multicast because every gate and the HUD listen */
DECLARE_MULTICAST_DELEGATE_OneParam(FStudySectionUnlocked, FName);

/* Fires with the id of the problem that just became active;
   multicast because the HUD and stations listen */
DECLARE_MULTICAST_DELEGATE_OneParam(FStudyActiveProblemChanged,
									const FString&);

/* Fires after the study clock was moved forward; multicast
   because every castle listens */
DECLARE_MULTICAST_DELEGATE(FStudyTimeSkipped);

/* Holds the state of the study world that every system shares */
UCLASS()
class MEMORYPALACE_API AStudyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	/* Sets default values for this game state */
	AStudyGameState();

	/* Unlocks a section and announces it, unless already open */
	void UnlockSection(FName SectionName);

	/* Returns whether a section has been unlocked */
	bool IsSectionUnlocked(FName SectionName) const;

	/* Records the active problem and announces the change */
	void SetActiveProblem(const FString& ProblemId);

	/* Returns the id of the problem being worked on */
	const FString& GetActiveProblemId() const;

	/* Moves the study clock forward, to show how unstudied
	   castles decay over time */
	void SkipStudyTime(const FTimespan& SkippedTime);

	/* Returns the current time on the study clock */
	FDateTime GetStudyNow() const;

	/* Fires when a section unlocks */
	FStudySectionUnlocked OnSectionUnlocked;

	/* Fires when the active problem changes */
	FStudyActiveProblemChanged OnActiveProblemChanged;

	/* Fires when the study clock moves forward */
	FStudyTimeSkipped OnStudyTimeSkipped;

private:
	/* Names of the sections the student can enter */
	TSet<FName> UnlockedSectionNames;

	/* Id of the problem being worked on, empty if none */
	FString ActiveProblemId;

	/* Time added to the real clock by the demo time skip */
	FTimespan TotalSkippedTime;
};
