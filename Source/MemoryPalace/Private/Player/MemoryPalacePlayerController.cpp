// Copyright Epic Games, Inc. All Rights Reserved.


#include "Player/MemoryPalacePlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Game/MemoryPalaceGameMode.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Interaction/StudyInteractable.h"
#include "Player/MemoryPalaceCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "MemoryPalace.h"
#include "Widgets/Input/SVirtualJoystick.h"

namespace
{
	constexpr int32 MappingContextPriority{0};
}

AMemoryPalacePlayerController::AMemoryPalacePlayerController()
{
	PlayerCameraManagerClass = AMemoryPalaceCameraManager::StaticClass();
}

void AMemoryPalacePlayerController::BeginPlay()
{
	Super::BeginPlay();


	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogMemoryPalace, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AMemoryPalacePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!IsLocalPlayerController())
	{
		return;
	}
	AddMappingContexts();
	BindStudyActions();
}

void AMemoryPalacePlayerController::AddMappingContexts()
{
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem{
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
			GetLocalPlayer())};
	if (!InputSubsystem)
	{
		return;
	}
	for (UInputMappingContext* MappingContext : DefaultMappingContexts)
	{
		InputSubsystem->AddMappingContext(
			MappingContext, MappingContextPriority);
	}
	if (ShouldUseTouchControls())
	{
		return;
	}
	for (UInputMappingContext* MappingContext :
		 MobileExcludedMappingContexts)
	{
		InputSubsystem->AddMappingContext(
			MappingContext, MappingContextPriority);
	}
}

void AMemoryPalacePlayerController::BindStudyActions()
{
	UEnhancedInputComponent* EnhancedInputComponent{
		Cast<UEnhancedInputComponent>(InputComponent)};
	if (!EnhancedInputComponent)
	{
		return;
	}
	if (InteractAction)
	{
		EnhancedInputComponent->BindAction(
			InteractAction, ETriggerEvent::Started, this,
			&AMemoryPalacePlayerController::HandleInteractPressed);
	}
	if (HintAction)
	{
		EnhancedInputComponent->BindAction(
			HintAction, ETriggerEvent::Started, this,
			&AMemoryPalacePlayerController::HandleHintPressed);
	}
}

void AMemoryPalacePlayerController::HandleHintPressed()
{
	AMemoryPalaceGameMode* GameMode{
		GetWorld()->GetAuthGameMode<AMemoryPalaceGameMode>()};
	if (!GameMode)
	{
		return;
	}
	GameMode->RequestHint();
}

void AMemoryPalacePlayerController::HandleInteractPressed()
{
	const APawn* ControlledPawn{GetPawn()};
	if (!ControlledPawn)
	{
		return;
	}
	TArray<AActor*> OverlappingActors;
	ControlledPawn->GetOverlappingActors(OverlappingActors);
	for (AActor* OverlappingActor : OverlappingActors)
	{
		IStudyInteractable* Interactable{
			Cast<IStudyInteractable>(OverlappingActor)};
		if (Interactable)
		{
			Interactable->Interact();
			return;
		}
	}
}

bool AMemoryPalacePlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
