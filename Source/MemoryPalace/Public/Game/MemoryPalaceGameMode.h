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

	/* Makes a problem the active one on the server and in the
	   game state */
	void StartProblem(const FString& ProblemId);

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

	/* Gives experience for the correct parts to the review's
	   subject and section, even if other parts were wrong */
	void AwardExperience(const FStudyReviewResult& ReviewResult);

	/* Adds up the experience of the correct parts and records
	   when each one was rewarded */
	double ClaimCorrectPartsExperience(
		const FStudyReviewResult& ReviewResult,
		AStudyPlayerState& StudyPlayerState);

	/* Returns 1 for a part never rewarded, less right after a
	   reward, recovering to 1 as time passes */
	double ComputeFreshness(
		const AStudyPlayerState& StudyPlayerState,
		const FString& PartKey) const;

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

	/* Share of a part's experience given when it is solved again
	   right away */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Rules")
	float MinimumRepeatFraction;

	/* Seconds until a solved part gives its full experience again */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Rules")
	float RepeatRecoverySeconds;

	/* Sections of the course and their unlock thresholds */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Rules")
	TObjectPtr<UStudyCurriculumDataAsset> Curriculum;

	// Cached Vars
	/* Subsystem this game mode is listening to */
	TWeakObjectPtr<UStudyPadSubsystem> CachedStudyPadSubsystem;
};
