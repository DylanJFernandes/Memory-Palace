// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConfirmDeleteWidget.generated.h"

class UButton;
class UTextBlock;

/* Asks the player to confirm that a world is deleted */
UCLASS()
class MEMORYPALACE_API UConfirmDeleteWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/* Shows which world is about to be deleted */
	void ShowWorld(const FString& WorldName);

	/* Fires when Delete is confirmed */
	FSimpleDelegate OnConfirmed;

	/* Fires when the player keeps the world */
	FSimpleDelegate OnCancelled;

protected:
	/* Listens to the buttons */
	virtual void NativeConstruct() override;

private:
	/* Announces that the deletion was confirmed */
	UFUNCTION()
	void HandleConfirmClicked();

	/* Announces that the deletion was cancelled */
	UFUNCTION()
	void HandleCancelClicked();

	/* Asks whether the world should really be deleted */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> QuestionText;

	/* Deletes the world */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ConfirmButton;

	/* Keeps the world */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CancelButton;
};
