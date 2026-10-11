// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/HUD/StudyHUD.h"
#include "Engine/World.h"
#include "Game/StudyPadSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Player/StudyPlayerState.h"
#include "UI/Widget/StudyExperienceWidget.h"
#include "UI/Widget/StudyFeedbackWidget.h"

void AStudyHUD::BeginPlay()
{
	Super::BeginPlay();
	CreateFeedbackWidget();
	CreateExperienceWidget();
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
	FeedbackWidget->Hide();
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
