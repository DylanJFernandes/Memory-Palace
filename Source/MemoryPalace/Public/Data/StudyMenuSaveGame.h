// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "StudyMenuSaveGame.generated.h"

/* Menu data kept between sessions: the worlds and the API key */
UCLASS()
class MEMORYPALACE_API UStudyMenuSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/* Names of the worlds the player created, oldest first */
	UPROPERTY()
	TArray<FString> WorldNames;

	/* Claude API key entered in the settings */
	UPROPERTY()
	FString ApiKey;
};
