// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/Actor.h"
#include "Interaction/StudyInteractable.h"
#include "Interaction/StudyPageable.h"
#include "StudyWhiteboardActor.generated.h"

class AMemoryPalacePlayerController;
class UBoxComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UStudyHistoryComponent;
class UStudyWhiteboardComponent;
class UWidgetComponent;
struct FHitResult;

/* One study station: a whiteboard that shows the study pad, a
   history of past submissions, and a start point for the
   problem. All of it works from a single trigger box */
UCLASS()
class MEMORYPALACE_API AStudyWhiteboardActor
	: public AActor,
	  public IStudyInteractable,
	  public IStudyPageable
{
	GENERATED_BODY()

public:
	/* Sets default values for this actor */
	AStudyWhiteboardActor();

	// Study Interactable
	/* Asks the game mode to start this station's problem */
	virtual void Interact() override;
	//~End of Study Interactable

	// Study Pageable
	/* Shows the next older submission */
	virtual void ShowPreviousPage() override;

	/* Shows the next newer submission */
	virtual void ShowNextPage() override;
	//~End of Study Pageable

protected:
	/* Connects the history and starts listening for the cursor
	   being held */
	virtual void BeginPlay() override;

	/* Stops listening for the cursor */
	virtual void EndPlay(
		const EEndPlayReason::Type EndPlayReason) override;

private:
	/* Keeps the trigger box and widgets answering the cursor's
	   traces as intended, whatever the Blueprint saved */
	void ConfigureCollision();

	/* Subscribes to the local player's cursor events */
	void BindToCursorEvents();

	/* Sets whether the player already stands in the trigger box
	   when play begins */
	void CheckPlayerAlreadyInside();

	/* Remembers that the cursor is held and updates the page */
	void HandleCursorShown();

	/* Remembers that the cursor was released and updates the
	   page */
	void HandleCursorHidden();

	/* Remembers that the player is in the trigger box */
	UFUNCTION()
	void HandleTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	/* Remembers that the player left the trigger box and resets the
	   history to the newest submission */
	UFUNCTION()
	void HandleTriggerEndOverlap(
		UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);

	/* Shows the page only while the cursor is held and the player
	   stands in this station's trigger box */
	void UpdateStudyPageVisibility();

	/* Returns whether the actor is the local player's pawn */
	bool IsLocalPlayerPawn(const AActor* Actor) const;

	/* Shows the study pad page in front of the whiteboard */
	void ShowStudyPage();

	/* Hides the study pad page again */
	void HideStudyPage();

	/* Asks the widget component to draw the page again, since it
	   otherwise redraws only while the cursor is over it */
	void RequestStudyPageRedraw();

	/* Area the player stands in to use this whole station */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	/* Mesh that forms the physical whiteboard */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UStaticMeshComponent> WhiteboardMesh;

	/* Component that displays the live study pad image and holds
	   the problem id */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UStudyWhiteboardComponent> WhiteboardComponent;

	/* Component that draws the study pad page over the mesh */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UWidgetComponent> StudyPageWidgetComponent;

	/* Separate object that draws the submission history */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UWidgetComponent> HistoryWidgetComponent;

	/* Component that pages through the submissions */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UStudyHistoryComponent> HistoryComponent;

	/* Timer that keeps redrawing the page while it is shown */
	FTimerHandle RedrawTimerHandle;

	/* Whether the player is holding the cursor */
	bool bIsCursorHeld;

	/* Whether the player stands in the trigger box */
	bool bIsPlayerInside;

	// Cached Vars
	/* Controller whose cursor events this station follows */
	TWeakObjectPtr<AMemoryPalacePlayerController>
		CachedPlayerController;
};
