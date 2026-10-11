// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TitleScreenWidget.generated.h"

class UButton;

/* Title screen with Single Player, Multiplayer and Settings */
UCLASS()
class MEMORYPALACE_API UTitleScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/* Fires when Single Player is pressed */
	FSimpleDelegate OnSinglePlayerPressed;

	/* Fires when Settings is pressed */
	FSimpleDelegate OnSettingsPressed;

protected:
	/* Listens to the buttons and greys out Multiplayer */
	virtual void NativeConstruct() override;

private:
	/* Announces that Single Player was pressed */
	UFUNCTION()
	void HandleSinglePlayerClicked();

	/* Announces that Settings was pressed */
	UFUNCTION()
	void HandleSettingsClicked();

	/* Opens the world list */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SinglePlayerButton;

	/* Not available yet, so it stays greyed out */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MultiplayerButton;

	/* Opens the settings */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SettingsButton;
};
