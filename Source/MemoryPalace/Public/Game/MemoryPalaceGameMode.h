// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MemoryPalaceGameMode.generated.h"

/* Game mode that owns the rules of a study world */
UCLASS(abstract)
class AMemoryPalaceGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/* Sets default values for this game mode */
	AMemoryPalaceGameMode();

protected:
	/* Starts the study pad polling for this world */
	virtual void BeginPlay() override;
};
