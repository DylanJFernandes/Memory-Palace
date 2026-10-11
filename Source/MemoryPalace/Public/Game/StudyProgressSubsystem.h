// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "StudyProgressSubsystem.generated.h"

class AStudyPlayerState;
class UStudyProgressSaveGame;
struct FStudyReviewResult;
struct FStudySavedProblem;
struct FStudySavedSubmission;

/* Keeps the student's saved progress for this world: boards,
   submission history and experience */
UCLASS()
class MEMORYPALACE_API UStudyProgressSubsystem
	: public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/* Adds a graded submission to its problem's history */
	void RecordSubmission(const FStudyReviewResult& ReviewResult,
						  int32 ExperienceGained);

	/* Remembers the latest whiteboard PNG of a problem in memory;
	   it reaches the disk on the next save */
	void SetBoardImage(const FString& ProblemId,
					   const TArray<uint8>& PngBytes);

	/* Copies the player's experience into the save */
	void StoreExperience(const AStudyPlayerState& PlayerState);

	/* Gives the player the experience that was saved */
	void RestoreExperience(AStudyPlayerState& PlayerState) const;

	/* Writes everything remembered so far to this world's slot */
	void SaveToDisk() const;

	/* Returns the saved data of each problem, by problem id */
	const TMap<FString, FStudySavedProblem>& GetProblems() const;

protected:
	/* Creates the subsystem only for game worlds */
	virtual bool ShouldCreateSubsystem(
		UObject* Outer) const override;

	/* Loads this world's save slot, or starts a new one */
	virtual void Initialize(
		FSubsystemCollectionBase& Collection) override;

	/* Saves one last time when the world is torn down */
	virtual void Deinitialize() override;

private:
	/* Returns the name of the slot used by this world */
	FString BuildSlotName() const;

	/* Converts a graded review into a saved submission */
	static FStudySavedSubmission BuildSubmission(
		const FStudyReviewResult& ReviewResult,
		int32 ExperienceGained);

	/* Save data loaded from, and written back to, the slot */
	UPROPERTY()
	TObjectPtr<UStudyProgressSaveGame> ProgressSave;
};
