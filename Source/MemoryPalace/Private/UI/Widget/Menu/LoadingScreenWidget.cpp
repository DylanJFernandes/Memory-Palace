// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Menu/LoadingScreenWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void ULoadingScreenWidget::ShowProgress(
	float Progress, const FString& StatusMessage)
{
	LoadingProgressBar->SetPercent(Progress);
	StatusText->SetText(FText::FromString(StatusMessage));
}
