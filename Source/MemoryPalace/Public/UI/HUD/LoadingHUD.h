// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/HUD.h"
#include "LoadingHUD.generated.h"

class ULoadingScreenWidget;
class UStudyMenuSubsystem;
class UWorld;

/* Owns the loading screen, asks the server to build the world's
   problems and opens the world when they are ready */
UCLASS()
class MEMORYPALACE_API ALoadingHUD : public AHUD
{
	GENERATED_BODY()

public:
	/* Sets default values for this HUD */
	ALoadingHUD();

protected:
	/* Shows the loading screen and starts building */
	virtual void BeginPlay() override;

private:
	/* Creates the loading screen and adds it to the viewport */
	void CreateLoadingScreenWidget();

	/* Starts building and checks on it on a timer */
	void StartBuilding();

	/* Asks the server how far building has come */
	void RequestBuildStatus();

	/* Moves the bar and opens the world once it is ready */
	void HandleBuildStatusReceived(float Progress,
								   const FString& Status,
								   const FString& Message);

	/* Returns the bar's next value: the server's progress, or a
	   slow creep while the AI works, never reaching the end */
	float ComputeDisplayedProgress(float ServerProgress) const;

	/* Opens the study world after a short pause */
	void OpenGameLevelSoon(float DelaySeconds);

	/* Opens the study world */
	void OpenGameLevel();

	/* Returns the menu subsystem of the game instance */
	UStudyMenuSubsystem* GetMenuSubsystem() const;

	/* Seconds between build status checks */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Loading")
	float StatusIntervalSeconds;

	/* Level the study world is played in */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Loading")
	TSoftObjectPtr<UWorld> GameLevel;

	/* Widget class of the loading screen */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|UI")
	TSubclassOf<ULoadingScreenWidget> LoadingScreenWidgetClass;

	/* Loading screen created by this HUD */
	UPROPERTY()
	TObjectPtr<ULoadingScreenWidget> LoadingScreenWidget;

	/* Value the bar shows now */
	float DisplayedProgress;

	/* Whether the world is already being opened */
	bool bIsOpeningGameLevel;

	/* Timer that checks on building */
	FTimerHandle StatusTimerHandle;

	/* Timer that opens the world */
	FTimerHandle OpenLevelTimerHandle;
};
