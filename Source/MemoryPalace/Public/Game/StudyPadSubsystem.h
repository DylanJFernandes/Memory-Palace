// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Data/StudyReviewResult.h"
#include "StudyPadSubsystem.generated.h"

class FJsonObject;
class UTexture2D;

/* Fires with the newest whiteboard image and its width over
   height; multicast because every whiteboard listens */
DECLARE_MULTICAST_DELEGATE_TwoParams(FStudyWorkImageUpdated,
									 UTexture2D*, float);

/* Fires when the student submitted and the AI began reviewing;
   multicast because the HUD and the game mode both listen */
DECLARE_MULTICAST_DELEGATE(FStudyReviewStarted);

/* Fires when the AI's verdict arrives; multicast because the
   HUD and the game mode both listen */
DECLARE_MULTICAST_DELEGATE_OneParam(FStudyReviewFinished,
									const FStudyReviewResult&);

/* Fires with a message when the review could not be completed;
   multicast because the HUD and the game mode both listen */
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
   the AI's review once polling has been started */
UCLASS()
class MEMORYPALACE_API UStudyPadSubsystem
	: public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/* Sets default values for this subsystem */
	UStudyPadSubsystem();

	/* Starts asking the server for images and reviews */
	void StartPolling();

	/* Tells the server which problem is now active */
	void StartProblem(const FString& ProblemId);

	/* Returns the newest image, or null if none arrived yet */
	UTexture2D* GetLatestTexture() const;

	/* Returns the newest image's width over height */
	float GetLatestAspectRatio() const;

	/* Fires whenever a new image has been downloaded */
	FStudyWorkImageUpdated OnWorkImageUpdated;

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

	/* Stops polling when the world is torn down */
	virtual void Deinitialize() override;

private:
	/* Tells the server the game is closing, which ends the
	   active problem */
	void EndProblem();

	/* Sends a request for the latest image unless one is
	   already waiting for a response */
	void RequestLatestImage();

	/* Publishes the image when the server returned one */
	void HandleImageResponse(FHttpRequestPtr Request,
							 FHttpResponsePtr Response,
							 bool bConnectedSuccessfully);

	/* Decodes the PNG bytes and notifies listeners unless the
	   image is the same as the last one */
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

	/* Announces what the server reported, then marks the first
	   poll as done */
	void ApplyServerStatus(const FJsonObject& ResultObject);

	/* Announces the start of a review once per review */
	void HandleReviewing();

	/* Announces a finished or failed review once per review,
	   and stays silent for a leftover one on the first poll */
	void HandleOutcome(const FJsonObject& ResultObject);

	/* Fires the finished or failed event for the current status */
	void BroadcastOutcome(const FJsonObject& ResultObject);

	/* Builds a request to the study pad server */
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe>
		CreateServerRequest(const TCHAR* Verb,
							const FString& Path);

	/* Builds a key that is unique to each submitted review */
	static FString BuildResultKey(const FJsonObject& ResultObject);

	/* Converts the server's status text to a review status */
	static EStudyReviewStatus ParseStatus(
		const FString& StatusText);

	/* Whether polling has been started for this world */
	bool bIsPolling;

	/* Whether an image request is still waiting */
	bool bIsImageRequestInFlight;

	/* Whether a result request is still waiting */
	bool bIsResultRequestInFlight;

	/* Whether the first result poll has already come back */
	bool bHasPolledResult;

	/* Review status the server reported last */
	EStudyReviewStatus CurrentReviewStatus;

	/* Key of the last finished or failed review announced */
	FString LastHandledResultKey;

	/* Checksum of the last image that was decoded */
	uint32 LastImageChecksum;

	/* Width over height of the newest image */
	float LatestAspectRatio;

	/* Timer that triggers each image poll */
	FTimerHandle ImagePollTimerHandle;

	/* Timer that triggers each result poll */
	FTimerHandle ResultPollTimerHandle;

	/* Most recent decoded image, kept alive for listeners */
	UPROPERTY()
	TObjectPtr<UTexture2D> LatestTexture;
};
