// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StudyWhiteboardActor.generated.h"

class UStaticMeshComponent;
class UStudyWhiteboardComponent;

/* Placeable whiteboard that shows the live study pad image */
UCLASS()
class MEMORYPALACE_API AStudyWhiteboardActor : public AActor
{
	GENERATED_BODY()

public:
	/* Sets default values for this actor */
	AStudyWhiteboardActor();

private:
	/* Mesh that forms the physical whiteboard */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UStaticMeshComponent> WhiteboardMesh;

	/* Component that displays the live study pad image */
	UPROPERTY(VisibleAnywhere, Category = "Custom|Components")
	TObjectPtr<UStudyWhiteboardComponent> WhiteboardComponent;
};
