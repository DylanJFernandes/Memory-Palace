// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "StudyPadImageSubsystem.generated.h"

class UTexture2D;

/* Fires with the newest whiteboard image once it is decoded */
DECLARE_DELEGATE_OneParam(FStudyPadImageReceived, UTexture2D*);

/* Polls the study pad server for the live whiteboard image
   while a game world is running */
UCLASS()
class MEMORYPALACE_API UStudyPadImageSubsystem
	: public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/* Sets default values for this subsystem */
	UStudyPadImageSubsystem();

	/* Returns the newest image, or null if none arrived yet */
	UTexture2D* GetLatestTexture() const;

	/* Fires whenever a new image has been downloaded */
	FStudyPadImageReceived OnImageReceived;

protected:
	/* Creates the subsystem only for game worlds */
	virtual bool ShouldCreateSubsystem(
		UObject* Outer) const override;

	/* Starts polling once the world begins play */
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	/* Stops polling when the world is torn down */
	virtual void Deinitialize() override;

private:
	/* Sends a request for the latest image unless one is
	   already waiting for a response */
	void RequestLatestImage();

	/* Builds the GET request for the latest image */
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe>
		CreateLatestImageRequest();

	/* Publishes the image when the server returned one */
	void HandleImageResponse(FHttpRequestPtr Request,
							 FHttpResponsePtr Response,
							 bool bConnectedSuccessfully);

	/* Decodes the PNG bytes and notifies listeners */
	void PublishImage(const TArray<uint8>& ImageBytes);

	/* Whether a request is still waiting for a response */
	bool bIsRequestInFlight;

	/* Timer that triggers each poll */
	FTimerHandle PollTimerHandle;

	/* Most recent decoded image, kept alive for listeners */
	UPROPERTY()
	TObjectPtr<UTexture2D> LatestTexture;
};
