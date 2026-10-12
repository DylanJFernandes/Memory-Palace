// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class UButton;

/* In-game menu with Resume, Settings, Main Menu and Quit */
UCLASS()
class MEMORYPALACE_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/* Fires when Resume is pressed */
	FSimpleDelegate OnResumePressed;

	/* Fires when Settings is pressed */
	FSimpleDelegate OnSettingsPressed;

	/* Fires when Main Menu is pressed */
	FSimpleDelegate OnMainMenuPressed;

	/* Fires when Quit is pressed */
	FSimpleDelegate OnQuitPressed;

protected:
	/* Listens to the buttons */
	virtual void NativeConstruct() override;

private:
	/* Announces that Resume was pressed */
	UFUNCTION()
	void HandleResumeClicked();

	/* Announces that Settings was pressed */
	UFUNCTION()
	void HandleSettingsClicked();

	/* Announces that Main Menu was pressed */
	UFUNCTION()
	void HandleMainMenuClicked();

	/* Announces that Quit was pressed */
	UFUNCTION()
	void HandleQuitClicked();

	/* Closes the menu and returns to the game */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ResumeButton;

	/* Opens the settings */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SettingsButton;

	/* Leaves the world for the title screen */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MainMenuButton;

	/* Closes the game */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;
};
