// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/StudyReviewResult.h"
#include "StudyFeedbackWidget.generated.h"

class UTextBlock;

/* Shows the progress and outcome of the AI's review */
UCLASS()
class MEMORYPALACE_API UStudyFeedbackWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/* Shows that the AI is reviewing the submitted work */
	void ShowReviewing();

	/* Shows the verdict, the feedback and each graded part */
	void ShowResult(const FStudyReviewResult& ReviewResult);

	/* Shows why the review could not be completed */
	void ShowError(const FString& ErrorMessage);

	/* Shows a hint, or a short message while one is written */
	void ShowHint(const FString& HintText);

private:
	/* Replaces the three texts and makes the widget visible */
	void ShowTexts(const FString& Status, const FString& Feedback,
				   const FString& Details);

	/* Returns the headline for a finished review */
	static FString BuildStatusText(
		const FStudyReviewResult& ReviewResult);

	/* Returns one line per graded part, then the time taken */
	static FString BuildDetailsText(
		const FStudyReviewResult& ReviewResult);

	/* Returns the line describing one graded part */
	static FString BuildPartLine(const FStudyPartResult& Part);

	/* Headline such as Correct or Not quite */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusTextBlock;

	/* Written feedback from the AI */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> FeedbackTextBlock;

	/* Graded parts and the time taken */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DetailsTextBlock;
};
