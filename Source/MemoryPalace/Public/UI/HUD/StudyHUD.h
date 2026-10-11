// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/StudyReviewResult.h"
#include "Data/StudyTopicKind.h"
#include "GameFramework/HUD.h"
#include "StudyHUD.generated.h"

class AStudyPlayerState;
class UStudyExperienceWidget;
class UStudyFeedbackWidget;
class UStudyPadSubsystem;

/* Creates and owns the study widgets and feeds them live data */
UCLASS()
class MEMORYPALACE_API AStudyHUD : public AHUD
{
	GENERATED_BODY()

protected:
	/* Creates the widgets and starts listening for updates */
	virtual void BeginPlay() override;

	/* Stops listening for updates */
	virtual void EndPlay(
		const EEndPlayReason::Type EndPlayReason) override;

private:
	/* Creates the feedback widget, hidden until a review starts */
	void CreateFeedbackWidget();

	/* Creates the experience widget and adds it to the screen */
	void CreateExperienceWidget();

	/* Listens for review events from the study pad */
	void BindToStudyPadSubsystem();

	/* Listens for experience gained by the player */
	void BindToPlayerState();

	/* Shows that a review is under way */
	void HandleReviewStarted();

	/* Shows the finished review */
	void HandleReviewFinished(const FStudyReviewResult& ReviewResult);

	/* Shows why the review failed */
	void HandleReviewFailed(const FString& ErrorMessage);

	/* Shows a hint, or a message while one is written */
	void HandleHintUpdated(const FString& HintText);

	/* Shows a topic's new experience total */
	void HandleExperienceChanged(EStudyTopicKind TopicKind,
								 FName TopicName,
								 int32 TotalExperience);

	/* Widget class used for the feedback display */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|UI")
	TSubclassOf<UStudyFeedbackWidget> FeedbackWidgetClass;

	/* Widget class used for the experience bars */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|UI")
	TSubclassOf<UStudyExperienceWidget> ExperienceWidgetClass;

	/* Feedback widget created and owned by this HUD */
	UPROPERTY()
	TObjectPtr<UStudyFeedbackWidget> FeedbackWidget;

	/* Experience widget created and owned by this HUD */
	UPROPERTY()
	TObjectPtr<UStudyExperienceWidget> ExperienceWidget;

	// Cached Vars
	/* Subsystem this HUD is listening to */
	TWeakObjectPtr<UStudyPadSubsystem> CachedStudyPadSubsystem;

	/* Player state this HUD is listening to */
	TWeakObjectPtr<AStudyPlayerState> CachedStudyPlayerState;
};
