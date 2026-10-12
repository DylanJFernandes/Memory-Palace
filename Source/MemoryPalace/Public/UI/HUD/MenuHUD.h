// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/HUD.h"
#include "MenuHUD.generated.h"

class UConfirmDeleteWidget;
class UCreateWorldWidget;
class USettingsWidget;
class UStudyMenuSubsystem;
class UTexture2D;
class UTitleScreenWidget;
class UUserWidget;
class UWorldSelectWidget;
class UWorld;

/* Owns the title screen's widgets and moves between them */
UCLASS()
class MEMORYPALACE_API AMenuHUD : public AHUD
{
	GENERATED_BODY()

public:
	/* Sets default values for this HUD */
	AMenuHUD();

protected:
	/* Creates the screens and shows the title screen */
	virtual void BeginPlay() override;

private:
	/* Creates every screen and adds it to the viewport */
	void CreateScreens();

	/* Creates one screen, hidden, in the viewport */
	template <typename WidgetType>
	WidgetType* CreateScreen(TSubclassOf<WidgetType> WidgetClass);

	/* Listens to every screen's buttons */
	void BindScreenEvents();

	/* Listens to the menu subsystem's server answers */
	void BindMenuSubsystemEvents();

	/* Shows one screen and hides the others */
	void ShowScreen(UUserWidget* Screen);

	/* Shows the title screen */
	void ShowTitleScreen();

	/* Shows the world list */
	void ShowWorldSelect();

	/* Asks the player to confirm deleting a world */
	void ShowDeleteConfirmation(const FString& WorldName);

	/* Deletes the world that was asked about and shows the
	   updated list */
	void ConfirmDeleteWorld();

	/* Shows the settings with the saved key */
	void ShowSettings();

	/* Saves the key and returns to the title screen */
	void HandleApiKeySaved(const FString& ApiKey);

	/* Shows the create world screen and its QR code */
	void ShowCreateWorld();

	/* Leaves the create world screen for the world list */
	void CancelCreateWorld();

	/* Asks the server whether homework has been uploaded */
	void RequestUploadStatus();

	/* Shows the downloaded QR code */
	void HandleQrCodeReceived(UTexture2D* QrCodeTexture);

	/* Shows the upload's status message */
	void HandleUploadStatusReceived(float Progress,
									const FString& Status,
									const FString& Message);

	/* Saves the new world and opens the loading level */
	void CreateWorld(const FString& WorldName);

	/* Opens a saved world directly */
	void PlayWorld(const FString& WorldName);

	/* Returns the menu subsystem of the game instance */
	UStudyMenuSubsystem* GetMenuSubsystem() const;

	/* Seconds between upload status checks */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Menu")
	float UploadStatusIntervalSeconds;

	/* Level that builds the world and shows the loading bar */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Menu")
	TSoftObjectPtr<UWorld> LoadingLevel;

	/* Level the study world is played in */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|Menu")
	TSoftObjectPtr<UWorld> GameLevel;

	// Widget Classes
	/* Widget class of the title screen */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|UI")
	TSubclassOf<UTitleScreenWidget> TitleScreenWidgetClass;

	/* Widget class of the settings */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|UI")
	TSubclassOf<USettingsWidget> SettingsWidgetClass;

	/* Widget class of the world list */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|UI")
	TSubclassOf<UWorldSelectWidget> WorldSelectWidgetClass;

	/* Widget class of the create world screen */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|UI")
	TSubclassOf<UCreateWorldWidget> CreateWorldWidgetClass;

	/* Widget class of the delete confirmation */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|UI")
	TSubclassOf<UConfirmDeleteWidget> ConfirmDeleteWidgetClass;

	// Widgets
	/* Title screen created by this HUD */
	UPROPERTY()
	TObjectPtr<UTitleScreenWidget> TitleScreenWidget;

	/* Settings created by this HUD */
	UPROPERTY()
	TObjectPtr<USettingsWidget> SettingsWidget;

	/* World list created by this HUD */
	UPROPERTY()
	TObjectPtr<UWorldSelectWidget> WorldSelectWidget;

	/* Create world screen created by this HUD */
	UPROPERTY()
	TObjectPtr<UCreateWorldWidget> CreateWorldWidget;

	/* Delete confirmation created by this HUD */
	UPROPERTY()
	TObjectPtr<UConfirmDeleteWidget> ConfirmDeleteWidget;

	/* World the player is being asked to delete */
	FString PendingDeleteWorldName;

	/* Timer that checks the upload while creating a world */
	FTimerHandle UploadStatusTimerHandle;
};
