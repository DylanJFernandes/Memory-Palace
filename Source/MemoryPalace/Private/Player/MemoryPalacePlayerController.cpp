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
#include "Interaction/StudyPageable.h"
#include "Player/MemoryPalaceCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "Component/StudyCursorInteractionComponent.h"
#include "MemoryPalace.h"
#include "Widgets/Input/SVirtualJoystick.h"

namespace
{
	constexpr int32 MappingContextPriority{0};
}

AMemoryPalacePlayerController::AMemoryPalacePlayerController()
	: bIsCursorActive{false}
{
	PlayerCameraManagerClass = AMemoryPalaceCameraManager::StaticClass();

	CursorInteractionComponent =
		CreateDefaultSubobject<UStudyCursorInteractionComponent>(
			TEXT("CursorInteractionComponent"));
	CursorInteractionComponent->SetupAttachment(RootComponent);
}

bool AMemoryPalacePlayerController::InputKey(
	const FInputKeyEventArgs& Params)
{
	const bool bIsKeyPress{
		Params.Event == IE_Pressed || Params.Event == IE_Repeat};
	const bool bIsKeyboardKey{
		!Params.Key.IsMouseButton() && !Params.Key.IsGamepadKey()
		&& !Params.Key.IsModifierKey()};
	if (!bIsCursorActive || !bIsKeyPress || !bIsKeyboardKey)
	{
		return Super::InputKey(Params);
	}
	CursorInteractionComponent->TypeKey(Params.Key);
	return true;
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
	BindPageKeys();
	BindCursorForwarding();
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
	if (CursorAction)
	{
		EnhancedInputComponent->BindAction(
			CursorAction, ETriggerEvent::Started, this,
			&AMemoryPalacePlayerController::HandleCursorPressed);
		EnhancedInputComponent->BindAction(
			CursorAction, ETriggerEvent::Completed, this,
			&AMemoryPalacePlayerController::HandleCursorReleased);
	}
}

void AMemoryPalacePlayerController::BindPageKeys()
{
	InputComponent->BindKey(
		EKeys::Left, IE_Pressed, this,
		&AMemoryPalacePlayerController::HandlePreviousPagePressed);
	InputComponent->BindKey(
		EKeys::Right, IE_Pressed, this,
		&AMemoryPalacePlayerController::HandleNextPagePressed);
}

void AMemoryPalacePlayerController::HandlePreviousPagePressed()
{
	IStudyPageable* Pageable{FindOverlappingPageable()};
	if (!Pageable)
	{
		return;
	}
	Pageable->ShowPreviousPage();
}

void AMemoryPalacePlayerController::HandleNextPagePressed()
{
	IStudyPageable* Pageable{FindOverlappingPageable()};
	if (!Pageable)
	{
		return;
	}
	Pageable->ShowNextPage();
}

IStudyPageable*
	AMemoryPalacePlayerController::FindOverlappingPageable() const
{
	const APawn* ControlledPawn{GetPawn()};
	if (!ControlledPawn)
	{
		return nullptr;
	}
	TArray<AActor*> OverlappingActors;
	ControlledPawn->GetOverlappingActors(OverlappingActors);
	for (AActor* OverlappingActor : OverlappingActors)
	{
		IStudyPageable* Pageable{
			Cast<IStudyPageable>(OverlappingActor)};
		if (Pageable)
		{
			return Pageable;
		}
	}
	return nullptr;
}

void AMemoryPalacePlayerController::HandleCursorPressed()
{
	bIsCursorActive = true;
	SetShowMouseCursor(true);
	SetIgnoreLookInput(true);
	SetIgnoreMoveInput(true);
	FInputModeGameAndUI CursorInputMode;
	CursorInputMode.SetHideCursorDuringCapture(false);
	CursorInputMode.SetLockMouseToViewportBehavior(
		EMouseLockMode::DoNotLock);
	SetInputMode(CursorInputMode);
	OnCursorShown.Broadcast();
}

void AMemoryPalacePlayerController::HandleCursorReleased()
{
	CursorInteractionComponent->ReleaseCursorButton();
	bIsCursorActive = false;
	SetShowMouseCursor(false);
	SetIgnoreLookInput(false);
	SetIgnoreMoveInput(false);
	SetInputMode(FInputModeGameOnly{});
	OnCursorHidden.Broadcast();
}

void AMemoryPalacePlayerController::BindCursorForwarding()
{
	InputComponent->BindKey(
		EKeys::LeftMouseButton, IE_Pressed, this,
		&AMemoryPalacePlayerController::HandleCursorButtonPressed);
	InputComponent->BindKey(
		EKeys::LeftMouseButton, IE_Released, this,
		&AMemoryPalacePlayerController::HandleCursorButtonReleased);
	InputComponent->BindAxisKey(
		EKeys::MouseWheelAxis, this,
		&AMemoryPalacePlayerController::HandleCursorWheel);
}

void AMemoryPalacePlayerController::HandleCursorButtonPressed()
{
	if (!bIsCursorActive)
	{
		return;
	}
	CursorInteractionComponent->PressCursorButton();
}

void AMemoryPalacePlayerController::HandleCursorButtonReleased()
{
	CursorInteractionComponent->ReleaseCursorButton();
}

void AMemoryPalacePlayerController::HandleCursorWheel(
	float WheelDelta)
{
	if (!bIsCursorActive || FMath::IsNearlyZero(WheelDelta))
	{
		return;
	}
	CursorInteractionComponent->ScrollCursorWheel(WheelDelta);
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
