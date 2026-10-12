// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WorldEntryWidget.generated.h"

class UButton;
class UTextBlock;

/* Fires with the name of a world */
DECLARE_DELEGATE_OneParam(FStudyWorldChosen, const FString&);

/* One row of the world list */
UCLASS()
class MEMORYPALACE_API UWorldEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/* Shows a world's name on this row */
	void ShowWorld(const FString& InWorldName);

	/* Fires when Play is pressed */
	FStudyWorldChosen OnWorldChosen;

	/* Fires when Delete is pressed */
	FStudyWorldChosen OnDeleteRequested;

protected:
	/* Listens to the Play button */
	virtual void NativeConstruct() override;

private:
	/* Announces this row's world */
	UFUNCTION()
	void HandlePlayClicked();

	/* Announces that this row's world should be deleted */
	UFUNCTION()
	void HandleDeleteClicked();

	/* Name of the world */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WorldNameText;

	/* Opens the world */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> PlayButton;

	/* Asks to delete the world */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> DeleteButton;

	/* World shown on this row */
	FString WorldName;
};
