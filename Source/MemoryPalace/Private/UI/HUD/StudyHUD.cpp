// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/HUD/StudyHUD.h"
#include "Engine/World.h"
#include "Game/StudyGameState.h"
#include "Game/StudyPadSubsystem.h"
#include "Game/StudyProgressSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Player/StudyPlayerState.h"
#include "UI/Widget/StudyExperienceWidget.h"
#include "UI/Widget/StudyFeedbackWidget.h"
#include "UI/Widget/StudyGradeHistoryWidget.h"

void AStudyHUD::BeginPlay()
{
	Super::BeginPlay();
	CreateFeedbackWidget();
	CreateExperienceWidget();
	CreateGradeHistoryWidget();
	BindToStudyPadSubsystem();
	BindToPlayerState();
}

void AStudyHUD::CreateFeedbackWidget()
{
	FeedbackWidget = CreateWidget<UStudyFeedbackWidget>(
		GetOwningPlayerController(), FeedbackWidgetClass);
	if (!FeedbackWidget)
	{
		return;
	}
	FeedbackWidget->SetVisibility(ESlateVisibility::Collapsed);
	FeedbackWidget->AddToViewport();
}

void AStudyHUD::CreateExperienceWidget()
{
	ExperienceWidget = CreateWidget<UStudyExperienceWidget>(
		GetOwningPlayerController(), ExperienceWidgetClass);
	if (!ExperienceWidget)
	{
		return;
	}
	ExperienceWidget->AddToViewport();
}

void AStudyHUD::CreateGradeHistoryWidget()
{
	GradeHistoryWidget = CreateWidget<UStudyGradeHistoryWidget>(
		GetOwningPlayerController(), GradeHistoryWidgetClass);
	if (!GradeHistoryWidget)
	{
		return;
	}
	GradeHistoryWidget->SetVisibility(ESlateVisibility::Collapsed);
	GradeHistoryWidget->AddToViewport();
}

void AStudyHUD::ToggleGradeHistory()
{
	if (!GradeHistoryWidget)
	{
		return;
	}
	if (GradeHistoryWidget->IsVisible())
	{
		GradeHistoryWidget->SetVisibility(
			ESlateVisibility::Collapsed);
		return;
	}
	RefreshGradeHistory();
	GradeHistoryWidget->SetVisibility(ESlateVisibility::Visible);
}

void AStudyHUD::RefreshGradeHistory()
{
	const AStudyGameState* StudyGameState{
		GetWorld()->GetGameState<AStudyGameState>()};
	const UStudyProgressSubsystem* ProgressSubsystem{
		GetWorld()->GetSubsystem<UStudyProgressSubsystem>()};
	if (!StudyGameState || !ProgressSubsystem)
	{
		return;
	}
	const FString& ProblemId{StudyGameState->GetActiveProblemId()};
	const FStudySavedProblem* SavedProblem{
		ProgressSubsystem->GetProblems().Find(ProblemId)};
	TArray<FStudySavedSubmission> Submissions;
	if (SavedProblem)
	{
		Submissions = SavedProblem->Submissions;
	}
	GradeHistoryWidget->ShowSubmissions(ProblemId, Submissions);
}

void AStudyHUD::BindToStudyPadSubsystem()
{
	UStudyPadSubsystem* StudyPadSubsystem{
		GetWorld()->GetSubsystem<UStudyPadSubsystem>()};
	if (!StudyPadSubsystem)
	{
		return;
	}
	CachedStudyPadSubsystem = StudyPadSubsystem;
	StudyPadSubsystem->OnReviewStarted.AddUObject(
		this, &AStudyHUD::HandleReviewStarted);
	StudyPadSubsystem->OnReviewFinished.AddUObject(
		this, &AStudyHUD::HandleReviewFinished);
	StudyPadSubsystem->OnReviewFailed.AddUObject(
		this, &AStudyHUD::HandleReviewFailed);
	StudyPadSubsystem->OnHintUpdated.BindUObject(
		this, &AStudyHUD::HandleHintUpdated);
}

void AStudyHUD::BindToPlayerState()
{
	const APlayerController* PlayerController{
		GetOwningPlayerController()};
	if (!PlayerController)
	{
		return;
	}
	AStudyPlayerState* StudyPlayerState{
		PlayerController->GetPlayerState<AStudyPlayerState>()};
	if (!StudyPlayerState)
	{
		return;
	}
	CachedStudyPlayerState = StudyPlayerState;
	StudyPlayerState->OnExperienceChanged.BindUObject(
		this, &AStudyHUD::HandleExperienceChanged);
	StudyPlayerState->AnnounceAllExperience();
}

void AStudyHUD::HandleReviewStarted()
{
	if (!FeedbackWidget)
	{
		return;
	}
	FeedbackWidget->ShowReviewing();
}

void AStudyHUD::HandleReviewFinished(
	const FStudyReviewResult& ReviewResult)
{
	if (!FeedbackWidget)
	{
		return;
	}
	FeedbackWidget->ShowResult(ReviewResult);
}

void AStudyHUD::HandleReviewFailed(const FString& ErrorMessage)
{
	if (!FeedbackWidget)
	{
		return;
	}
	FeedbackWidget->ShowError(ErrorMessage);
}

void AStudyHUD::HandleHintUpdated(const FString& HintText)
{
	if (!FeedbackWidget)
	{
		return;
	}
	FeedbackWidget->ShowHint(HintText);
}

void AStudyHUD::HandleExperienceChanged(
	EStudyTopicKind TopicKind, FName TopicName,
	int32 TotalExperience)
{
	if (!ExperienceWidget)
	{
		return;
	}
	ExperienceWidget->SetExperience(
		TopicKind, TopicName, TotalExperience);
}

void AStudyHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CachedStudyPadSubsystem.IsValid())
	{
		CachedStudyPadSubsystem->OnReviewStarted.RemoveAll(this);
		CachedStudyPadSubsystem->OnReviewFinished.RemoveAll(this);
		CachedStudyPadSubsystem->OnReviewFailed.RemoveAll(this);
		CachedStudyPadSubsystem->OnHintUpdated.Unbind();
	}
	if (CachedStudyPlayerState.IsValid())
	{
		CachedStudyPlayerState->OnExperienceChanged.Unbind();
	}
	Super::EndPlay(EndPlayReason);
}
