// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StudyFeedbackWidget.generated.h"

class UTextBlock;

/* Shows that a review is under way, why it failed, and hints */
UCLASS()
class MEMORYPALACE_API UStudyFeedbackWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/* Shows that the AI is reviewing the submitted work */
	void ShowReviewing();

	/* Hides the widget once the review has finished */
	void Hide();

	/* Shows why the review could not be completed */
	void ShowError(const FString& ErrorMessage);

	/* Shows a hint, or a short message while one is written */
	void ShowHint(const FString& HintText);

private:
	/* Replaces the two texts and makes the widget visible */
	void ShowTexts(const FString& Status, const FString& Feedback);

	/* Headline such as Reviewing or Hint */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusTextBlock;

	/* Message below the headline */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> FeedbackTextBlock;
};
