// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ArrowComponent.h"
#include "ProblemSlotComponent.generated.h"

/* Marks a spot in a castle where a problem can be placed, and
   which way it faces */
UCLASS(meta = (BlueprintSpawnableComponent))
class MEMORYPALACE_API UProblemSlotComponent : public UArrowComponent
{
	GENERATED_BODY()
};
