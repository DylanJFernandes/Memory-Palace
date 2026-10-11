// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/StudyReviewResult.h"
#include "GameFramework/GameModeBase.h"
#include "MemoryPalaceGameMode.generated.h"

class AStudyGameState;
class AStudyPlayerState;
class UStudyCurriculumDataAsset;
class UStudyPadSubsystem;
struct FStudySectionDefinition;

/* Game mode that owns the rules of a study world */
UCLASS(abstract)
class AMemoryPalaceGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/* Sets default values for this game mode */
	AMemoryPalaceGameMode();

protected:
	/* Listens for reviews and starts the study pad polling */
	virtual void BeginPlay() override;

	/* Stops listening for reviews */
	virtual void EndPlay(
		const EEndPlayReason::Type EndPlayReason) override;

private:
	/* Listens for finished reviews, then starts polling */
	void BindToStudyPadSubsystem();

	/* Unlocks every section that has no prerequisite */
	void UnlockStartingSections();

	/* Rewards a correct review and unlocks what it opened */
	void HandleReviewFinished(const FStudyReviewResult& ReviewResult);

	/* Gives experience to the review's subject and section */
	void AwardExperience(const FStudyReviewResult& ReviewResult);

	/* Unlocks every section whose prerequisite has enough
	   experience */
	void UnlockEligibleSections();

	/* Returns whether the prerequisite has enough experience */
	bool IsPrerequisiteMet(
		const FStudySectionDefinition& Section) const;

	/* Returns the first player's study player state, if any */
	AStudyPlayerState* GetStudyPlayerState() const;

	/* Experience given per point of the AI's difficulty rating */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Rules")
	int32 ExperiencePerDifficultyPoint;

	/* Sections of the course and their unlock thresholds */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Rules")
	TObjectPtr<UStudyCurriculumDataAsset> Curriculum;

	// Cached Vars
	/* Subsystem this game mode is listening to */
	TWeakObjectPtr<UStudyPadSubsystem> CachedStudyPadSubsystem;
};
