// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/StudyReviewResult.h"
#include "Data/StudyTopicKind.h"
#include "GameFramework/HUD.h"
#include "StudyHUD.generated.h"

class AStudyPlayerState;
class UPauseMenuWidget;
class USettingsWidget;
class UStudyExperienceWidget;
class UStudyFeedbackWidget;
class UStudyPadSubsystem;
class UWorld;

/* Creates and owns the study widgets and feeds them live data */
UCLASS()
class MEMORYPALACE_API AStudyHUD : public AHUD
{
	GENERATED_BODY()

public:
	/* Sets default values for this HUD */
	AStudyHUD();

	/* Shows the pause menu */
	void ShowPauseMenu();

	/* Hides the pause menu and the settings */
	void HidePauseMenu();

	/* Returns whether the pause menu or the settings are shown */
	bool GetIsPauseMenuOpen() const;

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

	/* Creates the pause menu, hidden until it is asked for */
	void CreatePauseMenuWidget();

	/* Creates the settings, hidden until the pause menu opens
	   them */
	void CreateSettingsWidget();

	/* Shows the pause menu and hides the settings */
	void ShowPauseMenuOnly();

	/* Closes the pause menu through the player controller, which
	   also gives the game its input back */
	void HandleResumePressed();

	/* Shows the settings with the saved key */
	void HandleSettingsPressed();

	/* Saves the key and returns to the pause menu */
	void HandleApiKeySaved(const FString& ApiKey);

	/* Leaves the world for the title screen */
	void HandleMainMenuPressed();

	/* Closes the game */
	void HandleQuitPressed();

	/* Listens for review events from the study pad */
	void BindToStudyPadSubsystem();

	/* Listens for experience gained by the player */
	void BindToPlayerState();

	/* Shows that a review is under way */
	void HandleReviewStarted();

	/* Hides the reviewing message, since the result is shown on
	   the history board */
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

	/* Widget class used for the pause menu */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|UI")
	TSubclassOf<UPauseMenuWidget> PauseMenuWidgetClass;

	/* Widget class used for the settings */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|UI")
	TSubclassOf<USettingsWidget> SettingsWidgetClass;

	/* Level of the title screen the Main Menu button opens */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Menu")
	TSoftObjectPtr<UWorld> TitleLevel;

	/* Feedback widget created and owned by this HUD */
	UPROPERTY()
	TObjectPtr<UStudyFeedbackWidget> FeedbackWidget;

	/* Experience widget created and owned by this HUD */
	UPROPERTY()
	TObjectPtr<UStudyExperienceWidget> ExperienceWidget;

	/* Pause menu created and owned by this HUD */
	UPROPERTY()
	TObjectPtr<UPauseMenuWidget> PauseMenuWidget;

	/* Settings created and owned by this HUD */
	UPROPERTY()
	TObjectPtr<USettingsWidget> SettingsWidget;

	/* Whether the pause menu or the settings are shown */
	bool bIsPauseMenuOpen;

	// Cached Vars
	/* Subsystem this HUD is listening to */
	TWeakObjectPtr<UStudyPadSubsystem> CachedStudyPadSubsystem;

	/* Player state this HUD is listening to */
	TWeakObjectPtr<AStudyPlayerState> CachedStudyPlayerState;
};
