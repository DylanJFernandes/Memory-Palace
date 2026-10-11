// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/StudyGradeHistoryWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UStudyGradeHistoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	PreviousButton->OnClicked.AddDynamic(
		this, &UStudyGradeHistoryWidget::HandlePreviousClicked);
	NextButton->OnClicked.AddDynamic(
		this, &UStudyGradeHistoryWidget::HandleNextClicked);
	StartNewButton->OnClicked.AddDynamic(
		this, &UStudyGradeHistoryWidget::HandleStartNewClicked);
}

void UStudyGradeHistoryWidget::HandleStartNewClicked()
{
	OnStartNewClicked.ExecuteIfBound();
}

void UStudyGradeHistoryWidget::HandlePreviousClicked()
{
	OnPreviousClicked.ExecuteIfBound();
}

void UStudyGradeHistoryWidget::HandleNextClicked()
{
	OnNextClicked.ExecuteIfBound();
}

void UStudyGradeHistoryWidget::SetProblemTitle(
	const FString& ProblemTitle)
{
	ProblemTitleTextBlock->SetText(FText::FromString(ProblemTitle));
}

void UStudyGradeHistoryWidget::ShowSubmission(
	const FStudySavedSubmission& Submission, int32 PageNumber,
	int32 PageCount)
{
	PageTextBlock->SetText(FText::FromString(
		BuildPageText(PageNumber, PageCount)));
	ScoreTextBlock->SetText(
		FText::FromString(BuildScoreText(Submission)));
	DetailsTextBlock->SetText(
		FText::FromString(BuildDetailsText(Submission)));
}

void UStudyGradeHistoryWidget::ShowEmptyPage()
{
	PageTextBlock->SetText(FText::FromString(TEXT("No submissions")));
	ScoreTextBlock->SetText(FText::GetEmpty());
	DetailsTextBlock->SetText(FText::FromString(
		TEXT("Submit your work to see your results here.")));
}

FString UStudyGradeHistoryWidget::BuildPageText(
	int32 PageNumber, int32 PageCount)
{
	const TCHAR* PreviousArrow{
		PageNumber > 1 ? TEXT("<") : TEXT(" ")};
	const TCHAR* NextArrow{
		PageNumber < PageCount ? TEXT(">") : TEXT(" ")};
	return FString::Printf(TEXT("%s  Submission %d / %d  %s"),
		PreviousArrow, PageNumber, PageCount, NextArrow);
}

FString UStudyGradeHistoryWidget::BuildScoreText(
	const FStudySavedSubmission& Submission)
{
	return FString::Printf(
		TEXT("%d / %d parts correct   +%d XP"),
		Submission.CorrectPartCount, Submission.PartCount,
		Submission.ExperienceGained);
}

FString UStudyGradeHistoryWidget::BuildDetailsText(
	const FStudySavedSubmission& Submission)
{
	TArray<FString> Lines;
	for (const FStudySavedPart& Part : Submission.Parts)
	{
		Lines.Add(FString::Printf(TEXT("%s: %s"), *Part.Label,
			Part.bIsCorrect ? TEXT("correct") : TEXT("not yet")));
	}
	Lines.Add(FString::Printf(
		TEXT("Time: %.0f seconds"), Submission.ElapsedSeconds));
	Lines.Add(Submission.Feedback);
	return FString::Join(Lines, TEXT("\n"));
}
