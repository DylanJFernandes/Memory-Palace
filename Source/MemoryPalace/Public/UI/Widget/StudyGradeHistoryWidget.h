// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/StudyProgressSaveGame.h"
#include "StudyGradeHistoryWidget.generated.h"

class UButton;
class UTextBlock;

/* Shows one problem's past submissions, one page each, with
   arrows to move between them */
UCLASS()
class MEMORYPALACE_API UStudyGradeHistoryWidget
	: public UUserWidget
{
	GENERATED_BODY()

public:
	/* Sets default values for this widget */
	UStudyGradeHistoryWidget(
		const FObjectInitializer& ObjectInitializer);

	/* Shows a problem's submissions, starting at the newest */
	void ShowSubmissions(
		const FString& ProblemTitle,
		const TArray<FStudySavedSubmission>& InSubmissions);

protected:
	/* Binds the arrow buttons */
	virtual void NativeConstruct() override;

private:
	/* Fills the texts with the submission on the current page */
	void RefreshPage();

	/* Shows the page text for a problem without submissions */
	void ShowEmptyPage();

	/* Enables each arrow only if there is a page in its
	   direction */
	void RefreshArrowButtons();

	/* Moves to the previous, older submission */
	UFUNCTION()
	void HandlePreviousClicked();

	/* Moves to the next, newer submission */
	UFUNCTION()
	void HandleNextClicked();

	/* Returns the headline with the score and experience */
	static FString BuildScoreText(
		const FStudySavedSubmission& Submission);

	/* Returns one line per part, then the time taken */
	static FString BuildDetailsText(
		const FStudySavedSubmission& Submission);

	/* Name of the problem being shown */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ProblemTitleTextBlock;

	/* Which submission is shown, such as 2 / 5 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PageTextBlock;

	/* Score and experience of the shown submission */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ScoreTextBlock;

	/* Parts, time and feedback of the shown submission */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DetailsTextBlock;

	/* Arrow that goes to the older submission */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> PreviousButton;

	/* Arrow that goes to the newer submission */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> NextButton;

	/* Index of the submission being shown */
	int32 CurrentPageIndex;

	/* Submissions of the problem, oldest first */
	TArray<FStudySavedSubmission> Submissions;
};
