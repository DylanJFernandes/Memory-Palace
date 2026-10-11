// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StudyHistoryComponent.generated.h"

class UStudyGradeHistoryWidget;
class UStudyProgressSubsystem;
class UStudyWhiteboardComponent;
class UWidgetComponent;
struct FStudySavedProblem;

/* Broadcast when the player asks for a fresh attempt */
DECLARE_DELEGATE(FStudyStartNewRequested);

/* Browses a problem's past submissions on a history widget, and
   shows the work of the selected submission on a whiteboard */
UCLASS()
class MEMORYPALACE_API UStudyHistoryComponent
	: public UActorComponent
{
	GENERATED_BODY()

public:
	/* Sets default values for this component */
	UStudyHistoryComponent();

	/* Connects the widget and whiteboard, and shows the newest
	   submission */
	void Initialize(
		UWidgetComponent* InHistoryWidgetComponent,
		UStudyWhiteboardComponent* InWhiteboardComponent);

	/* Shows the next older submission */
	void ShowPreviousPage();

	/* Shows the next newer submission */
	void ShowNextPage();

	/* Jumps to the newest submission, with the whiteboard back on
	   the live image */
	void ShowNewestPage();

	/* Fires when the start new attempt button is clicked */
	FStudyStartNewRequested OnStartNewRequested;

protected:
	/* Stops listening for new submissions */
	virtual void EndPlay(
		const EEndPlayReason::Type EndPlayReason) override;

private:
	/* Finds the history widget and binds its buttons */
	void BindHistoryWidget();

	/* Listens for saved submissions of the whiteboard's problem */
	void BindToProgressSubsystem();

	/* Moves the shown page by an offset, staying within the
	   saved submissions */
	void MovePage(int32 PageOffset);

	/* Makes the newest submission the shown page */
	void SelectNewestPage();

	/* Fills the widget with the current page, or the empty page */
	void ShowCurrentPage();

	/* Shows the current page's work on the whiteboard */
	void ShowCurrentWork();

	/* Returns the saved data of the whiteboard's problem */
	const FStudySavedProblem* FindSavedProblem() const;

	/* Jumps to the newest submission once a review is saved */
	void HandleSubmissionRecorded(const FString& ProblemId);

	/* Passes the start new attempt click on to the listener */
	void HandleStartNewClicked();

	/* Index of the shown submission, or none without any */
	int32 CurrentPageIndex;

	// Cached Vars
	/* Component that draws the history widget in the world */
	TWeakObjectPtr<UWidgetComponent> CachedHistoryWidgetComponent;

	/* Widget drawn by the widget component */
	TWeakObjectPtr<UStudyGradeHistoryWidget> CachedHistoryWidget;

	/* Whiteboard that shows the work of the current page */
	TWeakObjectPtr<UStudyWhiteboardComponent>
		CachedWhiteboardComponent;

	/* Subsystem that holds the saved submissions */
	TWeakObjectPtr<UStudyProgressSubsystem> CachedProgressSubsystem;
};
