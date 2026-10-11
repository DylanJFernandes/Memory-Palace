// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/MenuPlayerController.h"

AMenuPlayerController::AMenuPlayerController()
{
	PrimaryActorTick.bCanEverTick = false;
	bShowMouseCursor = true;
}

void AMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeUIOnly{});
}
