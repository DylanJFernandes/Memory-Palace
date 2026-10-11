// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MenuGameMode.generated.h"

/* Game mode of the title and loading levels: no pawn, a menu
   controller, and the HUD set in each level's Blueprint */
UCLASS()
class MEMORYPALACE_API AMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/* Sets the menu controller and removes the pawn */
	AMenuGameMode();
};
