// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/MemoryPalaceGameMode.h"
#include "Engine/World.h"
#include "Game/StudyPadSubsystem.h"

AMemoryPalaceGameMode::AMemoryPalaceGameMode()
{
}

void AMemoryPalaceGameMode::BeginPlay()
{
	Super::BeginPlay();
	UStudyPadSubsystem* StudyPadSubsystem{
		GetWorld()->GetSubsystem<UStudyPadSubsystem>()};
	if (!StudyPadSubsystem)
	{
		return;
	}
	StudyPadSubsystem->StartPolling();
}
