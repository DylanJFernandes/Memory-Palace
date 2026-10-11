// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Widget/Menu/WorldEntryWidget.h"
#include "WorldSelectWidget.generated.h"

class UButton;
class UScrollBox;

/* List of the player's worlds with Create World and Cancel */
UCLASS()
class MEMORYPALACE_API UWorldSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/* Fills the list with one row per world */
	void ShowWorlds(const TArray<FString>& WorldNames);

	/* Fires with the world whose Play was pressed */
	FStudyWorldChosen OnWorldChosen;

	/* Fires when Create World is pressed */
	FSimpleDelegate OnCreateWorldPressed;

	/* Fires when Cancel is pressed */
	FSimpleDelegate OnCancelPressed;

protected:
	/* Listens to the buttons */
	virtual void NativeConstruct() override;

private:
	/* Passes a row's chosen world on */
	void HandleWorldChosen(const FString& WorldName);

	/* Announces that Create World was pressed */
	UFUNCTION()
	void HandleCreateWorldClicked();

	/* Announces that Cancel was pressed */
	UFUNCTION()
	void HandleCancelClicked();

	/* Row widget created for each world */
	UPROPERTY(EditDefaultsOnly, Category = "Custom|UI")
	TSubclassOf<UWorldEntryWidget> WorldEntryWidgetClass;

	/* Holds the world rows */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> WorldListScrollBox;

	/* Opens the create world screen */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CreateWorldButton;

	/* Returns to the title screen */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CancelButton;
};
