// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/StudyInteractable.h"
#include "StudyProblemStation.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/* Spot in the level where the player starts a study problem */
UCLASS()
class MEMORYPALACE_API AStudyProblemStation
	: public AActor,
	  public IStudyInteractable
{
	GENERATED_BODY()

public:
	/* Sets default values for this station */
	AStudyProblemStation();

	// Study Interactable
	/* Asks the game mode to start this station's problem */
	virtual void Interact() override;
	//~End of Study Interactable

private:
	/* Area the player stands in to use the station */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	/* Mesh that marks where the station is */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UStaticMeshComponent> MarkerMesh;

	/* Id of the problem on the study pad server */
	UPROPERTY(EditAnywhere, Category = "Custom|Problem")
	FString ProblemId;
};
