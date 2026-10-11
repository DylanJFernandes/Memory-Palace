// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Data/StudyReviewResult.h"
#include "StudyPadImageSubsystem.generated.h"

class FJsonObject;
class UTexture2D;

/* Fires with the newest whiteboard image once it is decoded */
DECLARE_DELEGATE_OneParam(FStudyPadImageReceived, UTexture2D*);

/* Fires when the student submitted and the AI began reviewing */
DECLARE_MULTICAST_DELEGATE(FStudyReviewStarted);

/* Fires when the AI's verdict arrives */
DECLARE_MULTICAST_DELEGATE_OneParam(FStudyReviewFinished,
									const FStudyReviewResult&);

/* Fires with a message when the review could not be completed */
DECLARE_MULTICAST_DELEGATE_OneParam(FStudyReviewFailed,
									const FString&);

/* Stage of the review as last reported by the server */
enum class EStudyReviewStatus : uint8
{
	Idle,
	Reviewing,
	Finished,
	Failed
};

/* Polls the study pad server for the live whiteboard image and
   the AI's review while a game world is running */
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

	/* Fires when the AI starts reviewing a submission */
	FStudyReviewStarted OnReviewStarted;

	/* Fires when the AI's verdict is in */
	FStudyReviewFinished OnReviewFinished;

	/* Fires when the review failed or the server went away */
	FStudyReviewFailed OnReviewFailed;

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

	/* Publishes the image when the server returned one */
	void HandleImageResponse(FHttpRequestPtr Request,
							 FHttpResponsePtr Response,
							 bool bConnectedSuccessfully);

	/* Decodes the PNG bytes and notifies listeners */
	void PublishImage(const TArray<uint8>& ImageBytes);

	/* Sends a request for the latest review unless one is
	   already waiting for a response */
	void RequestLatestResult();

	/* Reads the review status out of the server's response */
	void HandleResultResponse(FHttpRequestPtr Request,
							  FHttpResponsePtr Response,
							  bool bConnectedSuccessfully);

	/* Reports a failure if a review was waiting on the server */
	void HandleResultUnavailable();

	/* Announces the status when it differs from the last one */
	void ApplyServerStatus(const FJsonObject& ResultObject);

	/* Fires the event that matches the current status */
	void BroadcastStatusChange(const FJsonObject& ResultObject);

	/* Builds a GET request to the study pad server */
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe>
		CreateGetRequest(const TCHAR* Path);

	/* Converts the server's status text to a review status */
	static EStudyReviewStatus ParseStatus(
		const FString& StatusText);

	/* Whether an image request is still waiting */
	bool bIsImageRequestInFlight;

	/* Whether a result request is still waiting */
	bool bIsResultRequestInFlight;

	/* Review status the server reported last */
	EStudyReviewStatus CurrentReviewStatus;

	/* Timer that triggers each image poll */
	FTimerHandle ImagePollTimerHandle;

	/* Timer that triggers each result poll */
	FTimerHandle ResultPollTimerHandle;

	/* Most recent decoded image, kept alive for listeners */
	UPROPERTY()
	TObjectPtr<UTexture2D> LatestTexture;
};
