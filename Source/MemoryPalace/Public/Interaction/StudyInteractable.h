// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StudyInteractable.generated.h"

/* Marks actors the player can use by pressing the interact key */
UINTERFACE(MinimalAPI)
class UStudyInteractable : public UInterface
{
	GENERATED_BODY()
};

/* Implemented by actors the player can use while standing in them */
class MEMORYPALACE_API IStudyInteractable
{
	GENERATED_BODY()

public:
	/* Performs the actor's action for the player */
	virtual void Interact() = 0;
};
