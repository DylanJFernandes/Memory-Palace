// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/StudyProgressSaveGame.h"
#include "StudyGradeHistoryWidget.generated.h"

class UButton;
class UTextBlock;

/* Broadcast when an arrow button is clicked */
DECLARE_DELEGATE(FStudyHistoryArrowClicked);

/* Shows the grade of one past submission of a problem, with
   arrow buttons to move between submissions */
UCLASS()
class MEMORYPALACE_API UStudyGradeHistoryWidget
	: public UUserWidget
{
	GENERATED_BODY()

public:
	/* Shows the name of the problem the history belongs to */
	void SetProblemTitle(const FString& ProblemTitle);

	/* Shows one submission and where it sits among all of them */
	void ShowSubmission(const FStudySavedSubmission& Submission,
						int32 PageNumber, int32 PageCount);

	/* Shows the text for a problem without submissions */
	void ShowEmptyPage();

	/* Fires when the arrow to the older submission is clicked */
	FStudyHistoryArrowClicked OnPreviousClicked;

	/* Fires when the arrow to the newer submission is clicked */
	FStudyHistoryArrowClicked OnNextClicked;

	/* Fires when the button to start a new attempt is clicked */
	FStudyHistoryArrowClicked OnStartNewClicked;

protected:
	/* Binds the arrow buttons */
	virtual void NativeConstruct() override;

private:
	/* Tells the listener the previous arrow was clicked */
	UFUNCTION()
	void HandlePreviousClicked();

	/* Tells the listener the next arrow was clicked */
	UFUNCTION()
	void HandleNextClicked();

	/* Tells the listener the start new attempt button was
	   clicked */
	UFUNCTION()
	void HandleStartNewClicked();

	/* Returns the page counter, with an arrow for each direction
	   that has a page */
	static FString BuildPageText(int32 PageNumber, int32 PageCount);

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

	/* Button that starts a fresh attempt at the problem */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> StartNewButton;
};
