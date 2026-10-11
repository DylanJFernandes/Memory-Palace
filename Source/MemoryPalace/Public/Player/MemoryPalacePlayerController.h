// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MemoryPalacePlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UUserWidget;

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

	/* Binds the interact action to the interact handler */
	void BindInteractAction();

	/* Uses the first interactable actor the pawn is standing in */
	void HandleInteractPressed();

	/* Action that uses the interactable actor the pawn is in */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Input")
	TObjectPtr<UInputAction> InteractAction;
};
