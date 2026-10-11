// Copyright Epic Games, Inc. All Rights Reserved.

#include "Component/StudyHistoryComponent.h"
#include "Component/StudyWhiteboardComponent.h"
#include "Components/WidgetComponent.h"
#include "Data/StudyProgressSaveGame.h"
#include "Engine/World.h"
#include "Game/StudyProgressSubsystem.h"
#include "UI/Widget/StudyGradeHistoryWidget.h"

UStudyHistoryComponent::UStudyHistoryComponent()
	: CurrentPageIndex{INDEX_NONE}
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStudyHistoryComponent::Initialize(
	UWidgetComponent* InHistoryWidgetComponent,
	UStudyWhiteboardComponent* InWhiteboardComponent)
{
	CachedHistoryWidgetComponent = InHistoryWidgetComponent;
	CachedWhiteboardComponent = InWhiteboardComponent;
	BindHistoryWidget();
	BindToProgressSubsystem();
	SelectNewestPage();
}

void UStudyHistoryComponent::BindHistoryWidget()
{
	if (!CachedHistoryWidgetComponent.IsValid()
		|| !CachedWhiteboardComponent.IsValid())
	{
		return;
	}
	CachedHistoryWidgetComponent->InitWidget();
	CachedHistoryWidget = Cast<UStudyGradeHistoryWidget>(
		CachedHistoryWidgetComponent->GetUserWidgetObject());
	if (!CachedHistoryWidget.IsValid())
	{
		return;
	}
	CachedHistoryWidget->OnPreviousClicked.BindUObject(
		this, &UStudyHistoryComponent::ShowPreviousPage);
	CachedHistoryWidget->OnNextClicked.BindUObject(
		this, &UStudyHistoryComponent::ShowNextPage);
	CachedHistoryWidget->OnStartNewClicked.BindUObject(
		this, &UStudyHistoryComponent::HandleStartNewClicked);
	CachedHistoryWidget->SetProblemTitle(
		CachedWhiteboardComponent->GetProblemId());
}

void UStudyHistoryComponent::BindToProgressSubsystem()
{
	CachedProgressSubsystem =
		GetWorld()->GetSubsystem<UStudyProgressSubsystem>();
	if (!CachedProgressSubsystem.IsValid())
	{
		return;
	}
	CachedProgressSubsystem->OnSubmissionRecorded.AddUObject(
		this, &UStudyHistoryComponent::HandleSubmissionRecorded);
}

void UStudyHistoryComponent::ShowPreviousPage()
{
	MovePage(-1);
}

void UStudyHistoryComponent::ShowNextPage()
{
	MovePage(1);
}

void UStudyHistoryComponent::MovePage(int32 PageOffset)
{
	const FStudySavedProblem* SavedProblem{FindSavedProblem()};
	if (!SavedProblem || SavedProblem->Submissions.IsEmpty())
	{
		return;
	}
	const int32 LastPageIndex{SavedProblem->Submissions.Num() - 1};
	CurrentPageIndex = FMath::Clamp(
		CurrentPageIndex + PageOffset, 0, LastPageIndex);
	ShowCurrentPage();
	ShowCurrentWork();
}

void UStudyHistoryComponent::ShowNewestPage()
{
	SelectNewestPage();
	ShowCurrentWork();
}

void UStudyHistoryComponent::SelectNewestPage()
{
	const FStudySavedProblem* SavedProblem{FindSavedProblem()};
	CurrentPageIndex =
		SavedProblem ? SavedProblem->Submissions.Num() - 1
					 : INDEX_NONE;
	ShowCurrentPage();
}

void UStudyHistoryComponent::ShowCurrentPage()
{
	if (!CachedHistoryWidget.IsValid())
	{
		return;
	}
	const FStudySavedProblem* SavedProblem{FindSavedProblem()};
	if (!SavedProblem
		|| !SavedProblem->Submissions.IsValidIndex(CurrentPageIndex))
	{
		CachedHistoryWidget->ShowEmptyPage();
	}
	else
	{
		CachedHistoryWidget->ShowSubmission(
			SavedProblem->Submissions[CurrentPageIndex],
			CurrentPageIndex + 1, SavedProblem->Submissions.Num());
	}
	CachedHistoryWidgetComponent->RequestRedraw();
}

void UStudyHistoryComponent::ShowCurrentWork()
{
	const FStudySavedProblem* SavedProblem{FindSavedProblem()};
	if (!CachedWhiteboardComponent.IsValid() || !SavedProblem
		|| !SavedProblem->Submissions.IsValidIndex(CurrentPageIndex))
	{
		return;
	}
	const bool bIsNewestPage{
		CurrentPageIndex == SavedProblem->Submissions.Num() - 1};
	if (bIsNewestPage)
	{
		CachedWhiteboardComponent->ShowLiveImage();
		return;
	}
	CachedWhiteboardComponent->ShowSavedImage(
		SavedProblem->Submissions[CurrentPageIndex]
			.BoardImagePngBytes);
}

const FStudySavedProblem* UStudyHistoryComponent::FindSavedProblem()
	const
{
	if (!CachedWhiteboardComponent.IsValid()
		|| !CachedProgressSubsystem.IsValid())
	{
		return nullptr;
	}
	return CachedProgressSubsystem->GetProblems().Find(
		CachedWhiteboardComponent->GetProblemId());
}

void UStudyHistoryComponent::HandleSubmissionRecorded(
	const FString& ProblemId)
{
	if (!CachedWhiteboardComponent.IsValid()
		|| CachedWhiteboardComponent->GetProblemId() != ProblemId)
	{
		return;
	}
	ShowNewestPage();
}

void UStudyHistoryComponent::HandleStartNewClicked()
{
	OnStartNewRequested.ExecuteIfBound();
}

void UStudyHistoryComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	if (CachedProgressSubsystem.IsValid())
	{
		CachedProgressSubsystem->OnSubmissionRecorded.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}
