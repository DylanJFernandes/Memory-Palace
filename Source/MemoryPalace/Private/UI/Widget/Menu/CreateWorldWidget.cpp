// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Menu/CreateWorldWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

namespace
{
	const FString DefaultWorldName{TEXT("My Palace")};
}

void UCreateWorldWidget::NativeConstruct()
{
	Super::NativeConstruct();
	CreateButton->OnClicked.AddDynamic(
		this, &UCreateWorldWidget::HandleCreateClicked);
	CancelButton->OnClicked.AddDynamic(
		this, &UCreateWorldWidget::HandleCancelClicked);
}

void UCreateWorldWidget::ShowQrCode(UTexture2D* QrCodeTexture)
{
	QrCodeImage->SetBrushFromTexture(QrCodeTexture);
}

void UCreateWorldWidget::ShowUploadStatus(
	const FString& StatusMessage)
{
	UploadStatusText->SetText(FText::FromString(StatusMessage));
}

void UCreateWorldWidget::HandleCreateClicked()
{
	const FString TypedName{
		WorldNameTextBox->GetText().ToString().TrimStartAndEnd()};
	OnCreatePressed.ExecuteIfBound(
		TypedName.IsEmpty() ? DefaultWorldName : TypedName);
}

void UCreateWorldWidget::HandleCancelClicked()
{
	OnCancelPressed.ExecuteIfBound();
}
