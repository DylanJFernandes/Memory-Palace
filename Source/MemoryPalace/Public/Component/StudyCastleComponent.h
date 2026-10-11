// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/StudyCurriculum.h"
#include "StudyCastleComponent.generated.h"

class AStudyGameState;
class AStudyWhiteboardActor;
class UNiagaraSystem;
class UProblemSlotComponent;
class USoundBase;
class UTextRenderComponent;
struct FStudyReviewResult;

/* Fires when every problem of a castle has been solved */
DECLARE_DELEGATE(FStudyCastleCompleted);

/* Turns a castle into one section of the course: fills its
   problem slots with whiteboards, labels it, keeps it locked
   and cracked until studied, and celebrates solved problems */
UCLASS()
class MEMORYPALACE_API UStudyCastleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/* Sets default values for this component */
	UStudyCastleComponent();

	/* Makes this castle hold a section; orbs fly towards the
	   target location when a problem is solved */
	void AssignSection(const FStudyCurriculumSection& InSection,
					   const FVector& InOrbTargetLocation);

	/* Returns whether every problem of the castle is solved */
	bool IsCompleted() const;

	/* Fires when the last problem of the castle is solved */
	FStudyCastleCompleted OnCastleCompleted;

private:
	/* Places one whiteboard per problem in slot order */
	void SpawnWhiteboards();

	/* Returns the castle's slots, ordered from the left of the
	   entrance around to the right of it */
	TArray<UProblemSlotComponent*> GetSlotsInEntranceOrder() const;

	/* Returns the middle of all slots in castle space */
	FVector FindSlotCenter(
		const TArray<UProblemSlotComponent*>& ProblemSlots) const;

	/* Returns a slot's location in castle space */
	FVector GetLocalLocation(
		const UProblemSlotComponent* ProblemSlot) const;

	/* Returns the angle of a slot around the slots' middle,
	   starting at the entrance and turning towards the left of
	   someone walking in */
	double GetAngleFromEntrance(
		const UProblemSlotComponent* ProblemSlot,
		const FVector& SlotCenter) const;

	/* Spawns the whiteboard of one problem at a slot */
	AStudyWhiteboardActor* SpawnWhiteboard(
		const FString& ProblemId,
		UProblemSlotComponent* ProblemSlot);

	/* Creates the section's name above the entrance */
	void CreateLabel();

	/* Listens for unlocks, reviews and the demo time skip */
	void BindToStudyEvents();

	/* Opens the castle when its section unlocks */
	void HandleSectionUnlocked(FName UnlockedSectionName);

	/* Lets the wooden door fall to the floor */
	void DropDoor();

	/* Records a solved problem of this castle and celebrates */
	void HandleReviewFinished(const FStudyReviewResult& ReviewResult);

	/* Sends experience orbs from a whiteboard to the target */
	void SpawnExperienceOrbs(const FString& ProblemId);

	/* Shoots the beacon and plays its sound */
	void CompleteCastle();

	/* Sets the crack amount of every castle material from how
	   well the section is remembered */
	void UpdateDecay();

	/* Returns how well the section is remembered, from 0 when
	   never studied to 1 when everything was just solved */
	double ComputeMastery() const;

	/* Returns how much of one solved problem is still
	   remembered on the study clock */
	double ComputeRetention(const FDateTime& SolveTime) const;

	/* Sets a scalar on every material of the castle's meshes */
	void SetCastleMaterialScalar(FName ParameterName, float Value);

	/* Plays a sound at the castle, if one is set */
	void PlaySoundAtCastle(USoundBase* Sound) const;

	/* Returns the world's study game state */
	AStudyGameState* GetStudyGameState() const;

	/* Whiteboard actor spawned in each problem slot */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Study")
	TSubclassOf<AStudyWhiteboardActor> WhiteboardClass;

	/* Particle effect of the orbs flying from a solved board */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Effects")
	TObjectPtr<UNiagaraSystem> ExperienceOrbSystem;

	/* Particle beam shot into the sky when the castle is done */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Effects")
	TObjectPtr<UNiagaraSystem> BeaconSystem;

	/* Sound played when a problem of this castle is solved */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Audio")
	TObjectPtr<USoundBase> SolvedSound;

	/* Sound played when the beacon fires */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Audio")
	TObjectPtr<USoundBase> BeaconSound;

	/* Sound played when the castle unlocks */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Audio")
	TObjectPtr<USoundBase> UnlockSound;

	/* Material scalar from 0 (clean) to 1 (fully cracked) */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Decay")
	FName CrackParameterName;

	/* Days until a solved problem is half forgotten */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Decay")
	double RetentionHalfLifeDays;

	/* Where the label sits relative to the castle's front */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Label")
	FVector LabelOffset;

	/* Height of the label's letters */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Label")
	float LabelTextSize;

	/* Section this castle holds */
	FStudyCurriculumSection Section;

	/* Location the experience orbs fly towards */
	FVector OrbTargetLocation;

	/* Whiteboard of each problem, by problem id */
	TMap<FString, TWeakObjectPtr<AStudyWhiteboardActor>>
		WhiteboardsByProblemId;

	/* Study clock time each problem was last solved */
	TMap<FString, FDateTime> SolveTimesByProblemId;

	/* Whether the section has been unlocked */
	bool bIsUnlocked;

	/* Whether the beacon has already fired */
	bool bHasFiredBeacon;

	/* Name of the section above the entrance */
	UPROPERTY()
	TObjectPtr<UTextRenderComponent> LabelComponent;
};
