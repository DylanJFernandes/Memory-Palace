// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StudyWhiteboardComponent.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UStudyPadSubsystem;
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
	   instance and remembers the mesh's starting scale */
	void CreateWhiteboardMaterial();

	/* Subscribes to the study pad subsystem and shows its
	   current image */
	void BindToStudyPadSubsystem();

	/* Applies a newly received image to the material and
	   reshapes the mesh to fit it */
	void HandleWorkImageUpdated(UTexture2D* WorkTexture,
								float AspectRatio);

	/* Stretches the mesh's local Y axis to the aspect ratio while
	   keeping its local Z axis as the height */
	void ApplyAspectRatio(float AspectRatio);

	/* Name of the material's texture parameter to set */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Whiteboard")
	FName TextureParameterName;

	/* Dynamic material that displays the whiteboard image */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> WhiteboardMaterial;

	/* Scale the mesh had before any image arrived */
	FVector InitialMeshScale;

	// Cached Vars
	/* Mesh that shows the whiteboard image */
	TWeakObjectPtr<UStaticMeshComponent> CachedWhiteboardMesh;

	/* Subsystem this component is subscribed to */
	TWeakObjectPtr<UStudyPadSubsystem> CachedStudyPadSubsystem;
};
