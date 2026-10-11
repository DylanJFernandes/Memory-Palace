// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MemoryPalacePlayerController.generated.h"

class IStudyPageable;
class UInputAction;
class UInputMappingContext;
class UStudyCursorInteractionComponent;
class UUserWidget;

/* Broadcast when the cursor becomes active or inactive */
DECLARE_MULTICAST_DELEGATE(FStudyCursorEvent);

/* First person player controller that sets up input and lets the
   player use interactable actors they stand in */
UCLASS(abstract, config="Game")
class MEMORYPALACE_API AMemoryPalacePlayerController
	: public APlayerController
{
	GENERATED_BODY()

public:
	/* Sets the camera manager class */
	AMemoryPalacePlayerController();

	/* Types the key into the world widget under the cursor while
	   the cursor is active, otherwise handles it as usual */
	virtual bool InputKey(
		const FInputKeyEventArgs& Params) override;

	/* Fires when the cursor becomes active */
	FStudyCursorEvent OnCursorShown;

	/* Fires when the cursor stops being active */
	FStudyCursorEvent OnCursorHidden;

protected:
	/* Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/* Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/* Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/* Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/* If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/* Gameplay initialization */
	virtual void BeginPlay() override;

	/* Adds the mapping contexts and binds the interact action */
	virtual void SetupInputComponent() override;

	/* Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

private:
	/* Adds the default mapping contexts for this player */
	void AddMappingContexts();

	/* Binds the interact and hint actions to their handlers */
	void BindStudyActions();

	/* Uses the first interactable actor the pawn is standing in */
	void HandleInteractPressed();

	/* Binds the left and right arrow keys to paging */
	void BindPageKeys();

	/* Flips the pageable actor the pawn is standing in back */
	void HandlePreviousPagePressed();

	/* Flips the pageable actor the pawn is standing in forward */
	void HandleNextPagePressed();

	/* Returns the first pageable actor the pawn is standing in */
	IStudyPageable* FindOverlappingPageable() const;

	/* Asks the game mode for a hint about the active problem */
	void HandleHintPressed();

	/* Shows the mouse cursor and stops the camera from turning */
	void HandleCursorPressed();

	/* Hides the mouse cursor and lets the camera turn again */
	void HandleCursorReleased();

	/* Binds the mouse button and wheel to the cursor widget
	   interaction */
	void BindCursorForwarding();

	/* Clicks the world widget under the cursor */
	void HandleCursorButtonPressed();

	/* Lets go of the click on the world widget */
	void HandleCursorButtonReleased();

	/* Scrolls the world widget under the cursor */
	void HandleCursorWheel(float WheelDelta);

	/* Action that uses the interactable actor the pawn is in */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Input")
	TObjectPtr<UInputAction> InteractAction;

	/* Action that asks for a hint */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Input")
	TObjectPtr<UInputAction> HintAction;

	/* Action held to use the mouse cursor on the screen */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Input")
	TObjectPtr<UInputAction> CursorAction;

	/* Component that clicks and types into world widgets */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UStudyCursorInteractionComponent>
		CursorInteractionComponent;

	/* Whether the cursor is currently active */
	bool bIsCursorActive;
};
