// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "StudyWhiteboardComponent.generated.h"

class UStudyPadImageSubsystem;
class UStudyWhiteboardWidget;
class UTexture2D;
class UWidgetComponent;

/* Shows the live study pad image on a world-space widget */
UCLASS()
class MEMORYPALACE_API UStudyWhiteboardComponent
	: public USceneComponent
{
	GENERATED_BODY()

public:
	/* Sets default values for this component */
	UStudyWhiteboardComponent();

protected:
	/* Creates the widget and listens for new images */
	virtual void BeginPlay() override;

	/* Stops listening for new images */
	virtual void EndPlay(
		const EEndPlayReason::Type EndPlayReason) override;

private:
	/* Creates and attaches the world-space widget component */
	void CreateWidgetComponent();

	/* Subscribes to the image subsystem and shows its image */
	void BindToImageSubsystem();

	/* Shows a newly received image on the widget */
	void HandleImageReceived(UTexture2D* ReceivedTexture);

	/* Widget class displayed on the whiteboard */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Whiteboard")
	TSubclassOf<UStudyWhiteboardWidget> WidgetClass;

	/* Size in pixels the widget is drawn at */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Whiteboard")
	FVector2D WidgetDrawSize;

	/* Widget component created when play begins */
	UPROPERTY()
	TObjectPtr<UWidgetComponent> WidgetComponent;

	// Cached Vars
	/* Subsystem this component is subscribed to */
	TWeakObjectPtr<UStudyPadImageSubsystem> CachedImageSubsystem;
};
