// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/StudyFeedbackWidget.h"
#include "Components/TextBlock.h"

void UStudyFeedbackWidget::ShowReviewing()
{
	ShowTexts(TEXT("Reviewing your work..."), FString{}, FString{});
}

void UStudyFeedbackWidget::ShowResult(
	const FStudyReviewResult& ReviewResult)
{
	ShowTexts(BuildStatusText(ReviewResult), ReviewResult.Feedback,
			  BuildDetailsText(ReviewResult));
}

void UStudyFeedbackWidget::ShowError(const FString& ErrorMessage)
{
	ShowTexts(TEXT("Review failed"), ErrorMessage, FString{});
}

void UStudyFeedbackWidget::ShowTexts(
	const FString& Status, const FString& Feedback,
	const FString& Details)
{
	StatusTextBlock->SetText(FText::FromString(Status));
	FeedbackTextBlock->SetText(FText::FromString(Feedback));
	DetailsTextBlock->SetText(FText::FromString(Details));
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

FString UStudyFeedbackWidget::BuildStatusText(
	const FStudyReviewResult& ReviewResult)
{
	if (ReviewResult.bIsPlaceholder)
	{
		return TEXT("Placeholder review (no API key set)");
	}
	if (ReviewResult.bIsCorrect)
	{
		return TEXT("Correct!");
	}
	return TEXT("Not quite");
}

FString UStudyFeedbackWidget::BuildDetailsText(
	const FStudyReviewResult& ReviewResult)
{
	TArray<FString> Lines;
	for (const FStudyPartResult& Part : ReviewResult.Parts)
	{
		Lines.Add(BuildPartLine(Part));
	}
	Lines.Add(FString::Printf(
		TEXT("Time: %.1f seconds"), ReviewResult.ElapsedSeconds));
	return FString::Join(Lines, TEXT("\n"));
}

FString UStudyFeedbackWidget::BuildPartLine(
	const FStudyPartResult& Part)
{
	if (Part.bIsCorrect)
	{
		return FString::Printf(TEXT("%s: correct"), *Part.Label);
	}
	const FString Written{Part.StudentAnswer.IsEmpty()
		? FString{TEXT("nothing")} : Part.StudentAnswer};
	return FString::Printf(
		TEXT("%s: wrong (you wrote %s, answer is %s)"),
		*Part.Label, *Written, *Part.CorrectAnswer);
}
