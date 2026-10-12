// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StudyPageable.generated.h"

/* Marks actors the player can page through with the arrow keys */
UINTERFACE(MinimalAPI)
class UStudyPageable : public UInterface
{
	GENERATED_BODY()
};

/* Implemented by actors with pages the player flips through
   while standing in them */
class MEMORYPALACE_API IStudyPageable
{
	GENERATED_BODY()

public:
	/* Moves to the previous page */
	virtual void ShowPreviousPage() = 0;

	/* Moves to the next page */
	virtual void ShowNextPage() = 0;
};
