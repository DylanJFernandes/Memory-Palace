// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Widget/Menu/WorldEntryWidget.h"
#include "CreateWorldWidget.generated.h"

class UButton;
class UEditableTextBox;
class UImage;
class UTextBlock;
class UTexture2D;

/* Screen to name a new world and upload its homework by QR
   code */
UCLASS()
class MEMORYPALACE_API UCreateWorldWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/* Shows the QR code of the upload page */
	void ShowQrCode(UTexture2D* QrCodeTexture);

	/* Shows how the upload is going */
	void ShowUploadStatus(const FString& StatusMessage);

	/* Fires with the typed name when Create New World is
	   pressed */
	FStudyWorldChosen OnCreatePressed;

	/* Fires when Cancel is pressed */
	FSimpleDelegate OnCancelPressed;

protected:
	/* Listens to the buttons */
	virtual void NativeConstruct() override;

private:
	/* Announces the new world's name, or a default name */
	UFUNCTION()
	void HandleCreateClicked();

	/* Announces that Cancel was pressed */
	UFUNCTION()
	void HandleCancelClicked();

	/* Field the world's name is typed into */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> WorldNameTextBox;

	/* QR code that opens the upload page on a phone */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> QrCodeImage;

	/* How the upload is going */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> UploadStatusText;

	/* Creates the world */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CreateButton;

	/* Returns to the world list */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CancelButton;
};
