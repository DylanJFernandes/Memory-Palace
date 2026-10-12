// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MenuPlayerController.generated.h"

/* Player controller of the title and loading levels: shows the
   mouse cursor and sends input to the UI only */
UCLASS()
class MEMORYPALACE_API AMenuPlayerController
	: public APlayerController
{
	GENERATED_BODY()

public:
	/* Shows the mouse cursor */
	AMenuPlayerController();

protected:
	/* Sends input to the UI only */
	virtual void BeginPlay() override;
};
