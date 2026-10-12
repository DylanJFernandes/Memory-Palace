// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/StudyFeedbackWidget.h"
#include "Components/TextBlock.h"

void UStudyFeedbackWidget::ShowReviewing()
{
	ShowTexts(TEXT("Reviewing your work..."), FString{});
}

void UStudyFeedbackWidget::Hide()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UStudyFeedbackWidget::ShowError(const FString& ErrorMessage)
{
	ShowTexts(TEXT("Review failed"), ErrorMessage);
}

void UStudyFeedbackWidget::ShowHint(const FString& HintText)
{
	ShowTexts(TEXT("Hint"), HintText);
}

void UStudyFeedbackWidget::ShowTexts(
	const FString& Status, const FString& Feedback)
{
	StatusTextBlock->SetText(FText::FromString(Status));
	FeedbackTextBlock->SetText(FText::FromString(Feedback));
	SetVisibility(ESlateVisibility::HitTestInvisible);
}
