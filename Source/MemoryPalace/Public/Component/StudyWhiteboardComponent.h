// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StudyWhiteboardComponent.generated.h"

class UMaterialInstanceDynamic;
class UStudyPadSubsystem;
class UTexture2D;

/* Shows one problem's study pad image on the owner's static
   mesh */
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

	/* Builds the 1x1 white texture used when there is no image */
	void CreateBlankTexture();

	/* Subscribes to the study pad subsystem and shows its
	   current image */
	void BindToStudyPadSubsystem();

	/* Applies a newly received image to the material */
	void HandleWorkImageUpdated(const FString& UpdatedProblemId,
								UTexture2D* WorkTexture,
								float AspectRatio);

	/* Applies an image to the material */
	void ShowTexture(UTexture2D* WorkTexture);

	/* Id of the problem whose work this whiteboard shows */
	UPROPERTY(EditAnywhere, Category = "Custom|Whiteboard")
	FString ProblemId;

	/* Name of the material's texture parameter to set */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Whiteboard")
	FName TextureParameterName;

	/* Plain white texture shown while no work image exists */
	UPROPERTY()
	TObjectPtr<UTexture2D> BlankTexture;

	/* Dynamic material that displays the whiteboard image */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> WhiteboardMaterial;

	// Cached Vars
	/* Subsystem this component is subscribed to */
	TWeakObjectPtr<UStudyPadSubsystem> CachedStudyPadSubsystem;
};
