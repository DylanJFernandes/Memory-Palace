// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/MenuGameMode.h"
#include "Player/MenuPlayerController.h"

AMenuGameMode::AMenuGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
	PlayerControllerClass = AMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}
