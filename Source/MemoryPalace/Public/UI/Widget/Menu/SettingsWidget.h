// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettingsWidget.generated.h"

class UButton;
class UEditableTextBox;

/* Fires with the API key the player typed */
DECLARE_DELEGATE_OneParam(FStudyApiKeySaved, const FString&);

/* Settings screen with a field for the Claude API key */
UCLASS()
class MEMORYPALACE_API USettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/* Fills the field with the saved key */
	void ShowApiKey(const FString& ApiKey);

	/* Fires when Save is pressed */
	FStudyApiKeySaved OnApiKeySaved;

	/* Fires when Back is pressed */
	FSimpleDelegate OnBackPressed;

protected:
	/* Listens to the buttons */
	virtual void NativeConstruct() override;

private:
	/* Announces the typed key */
	UFUNCTION()
	void HandleSaveClicked();

	/* Announces that Back was pressed */
	UFUNCTION()
	void HandleBackClicked();

	/* Field the API key is typed into */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> ApiKeyTextBox;

	/* Saves the key */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SaveButton;

	/* Returns to the title screen */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BackButton;
};
