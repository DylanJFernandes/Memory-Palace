// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Menu/ConfirmDeleteWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UConfirmDeleteWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ConfirmButton->OnClicked.AddDynamic(
		this, &UConfirmDeleteWidget::HandleConfirmClicked);
	CancelButton->OnClicked.AddDynamic(
		this, &UConfirmDeleteWidget::HandleCancelClicked);
}

void UConfirmDeleteWidget::ShowWorld(const FString& WorldName)
{
	QuestionText->SetText(FText::FromString(FString::Printf(
		TEXT("Are you sure you want to delete \"%s\"?"),
		*WorldName)));
}

void UConfirmDeleteWidget::HandleConfirmClicked()
{
	OnConfirmed.ExecuteIfBound();
}

void UConfirmDeleteWidget::HandleCancelClicked()
{
	OnCancelled.ExecuteIfBound();
}
