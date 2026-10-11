// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StudyWhiteboardComponent.generated.h"

class UMaterialInstanceDynamic;
class UStudyPadImageSubsystem;
class UTexture2D;

/* Shows the live study pad image on the owner's static mesh */
UCLASS()
class MEMORYPALACE_API UStudyWhiteboardComponent
	: public UActorComponent
{
	GENERATED_BODY()

public:
	/* Sets default values for this component */
	UStudyWhiteboardComponent();

protected:
	/* Creates the material and listens for new images */
	virtual void BeginPlay() override;

	/* Stops listening for new images */
	virtual void EndPlay(
		const EEndPlayReason::Type EndPlayReason) override;

private:
	/* Replaces the owner's first mesh material with a dynamic
	   instance */
	void CreateWhiteboardMaterial();

	/* Subscribes to the image subsystem and shows its image */
	void BindToImageSubsystem();

	/* Applies a newly received image to the material */
	void HandleImageReceived(UTexture2D* ReceivedTexture);

	/* Name of the material's texture parameter to set */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Whiteboard")
	FName TextureParameterName;

	/* Dynamic material that displays the whiteboard image */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> WhiteboardMaterial;

	// Cached Vars
	/* Subsystem this component is subscribed to */
	TWeakObjectPtr<UStudyPadImageSubsystem> CachedImageSubsystem;
};
