// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/StudyGradeHistoryWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

UStudyGradeHistoryWidget::UStudyGradeHistoryWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CurrentPageIndex{0}
{
}

void UStudyGradeHistoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	PreviousButton->OnClicked.AddDynamic(
		this, &UStudyGradeHistoryWidget::HandlePreviousClicked);
	NextButton->OnClicked.AddDynamic(
		this, &UStudyGradeHistoryWidget::HandleNextClicked);
}

void UStudyGradeHistoryWidget::ShowSubmissions(
	const FString& ProblemTitle,
	const TArray<FStudySavedSubmission>& InSubmissions)
{
	ProblemTitleTextBlock->SetText(FText::FromString(ProblemTitle));
	Submissions = InSubmissions;
	CurrentPageIndex = FMath::Max(Submissions.Num() - 1, 0);
	RefreshPage();
}

void UStudyGradeHistoryWidget::RefreshPage()
{
	RefreshArrowButtons();
	if (Submissions.IsEmpty())
	{
		ShowEmptyPage();
		return;
	}
	const FStudySavedSubmission& Submission{
		Submissions[CurrentPageIndex]};
	PageTextBlock->SetText(FText::FromString(FString::Printf(
		TEXT("Submission %d / %d"), CurrentPageIndex + 1,
		Submissions.Num())));
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

void UStudyGradeHistoryWidget::RefreshArrowButtons()
{
	PreviousButton->SetIsEnabled(CurrentPageIndex > 0);
	NextButton->SetIsEnabled(
		CurrentPageIndex < Submissions.Num() - 1);
}

void UStudyGradeHistoryWidget::HandlePreviousClicked()
{
	CurrentPageIndex = FMath::Max(CurrentPageIndex - 1, 0);
	RefreshPage();
}

void UStudyGradeHistoryWidget::HandleNextClicked()
{
	const int32 LastPageIndex{FMath::Max(Submissions.Num() - 1, 0)};
	CurrentPageIndex =
		FMath::Min(CurrentPageIndex + 1, LastPageIndex);
	RefreshPage();
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
