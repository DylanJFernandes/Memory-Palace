// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/HUD/StudyHUD.h"
#include "Engine/World.h"
#include "Game/StudyPadSubsystem.h"
#include "Engine/GameInstance.h"
#include "Game/StudyMenuSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Player/MemoryPalacePlayerController.h"
#include "Player/StudyPlayerState.h"
#include "UI/Widget/Menu/PauseMenuWidget.h"
#include "UI/Widget/Menu/SettingsWidget.h"
#include "UI/Widget/StudyExperienceWidget.h"
#include "UI/Widget/StudyFeedbackWidget.h"

namespace
{
	/* Draws the pause menu above the other study widgets */
	constexpr int32 PauseMenuZOrder{10};
}

AStudyHUD::AStudyHUD()
	: bIsPauseMenuOpen{false}
{
}

void AStudyHUD::BeginPlay()
{
	Super::BeginPlay();
	CreateFeedbackWidget();
	CreateExperienceWidget();
	CreatePauseMenuWidget();
	CreateSettingsWidget();
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

void AStudyHUD::CreatePauseMenuWidget()
{
	PauseMenuWidget = CreateWidget<UPauseMenuWidget>(
		GetOwningPlayerController(), PauseMenuWidgetClass);
	if (!PauseMenuWidget)
	{
		return;
	}
	PauseMenuWidget->OnResumePressed.BindUObject(
		this, &AStudyHUD::HandleResumePressed);
	PauseMenuWidget->OnSettingsPressed.BindUObject(
		this, &AStudyHUD::HandleSettingsPressed);
	PauseMenuWidget->OnMainMenuPressed.BindUObject(
		this, &AStudyHUD::HandleMainMenuPressed);
	PauseMenuWidget->OnQuitPressed.BindUObject(
		this, &AStudyHUD::HandleQuitPressed);
	PauseMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	PauseMenuWidget->AddToViewport(PauseMenuZOrder);
}

void AStudyHUD::CreateSettingsWidget()
{
	SettingsWidget = CreateWidget<USettingsWidget>(
		GetOwningPlayerController(), SettingsWidgetClass);
	if (!SettingsWidget)
	{
		return;
	}
	SettingsWidget->OnApiKeySaved.BindUObject(
		this, &AStudyHUD::HandleApiKeySaved);
	SettingsWidget->OnBackPressed.BindUObject(
		this, &AStudyHUD::ShowPauseMenuOnly);
	SettingsWidget->SetVisibility(ESlateVisibility::Collapsed);
	SettingsWidget->AddToViewport(PauseMenuZOrder);
}

void AStudyHUD::ShowPauseMenu()
{
	bIsPauseMenuOpen = true;
	ShowPauseMenuOnly();
}

void AStudyHUD::ShowPauseMenuOnly()
{
	if (!PauseMenuWidget || !SettingsWidget)
	{
		return;
	}
	SettingsWidget->SetVisibility(ESlateVisibility::Collapsed);
	PauseMenuWidget->SetVisibility(ESlateVisibility::Visible);
}

void AStudyHUD::HidePauseMenu()
{
	bIsPauseMenuOpen = false;
	if (!PauseMenuWidget || !SettingsWidget)
	{
		return;
	}
	PauseMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	SettingsWidget->SetVisibility(ESlateVisibility::Collapsed);
}

bool AStudyHUD::GetIsPauseMenuOpen() const
{
	return bIsPauseMenuOpen;
}

void AStudyHUD::HandleResumePressed()
{
	AMemoryPalacePlayerController* PlayerController{
		Cast<AMemoryPalacePlayerController>(
			GetOwningPlayerController())};
	if (!PlayerController)
	{
		return;
	}
	PlayerController->ClosePauseMenu();
}

void AStudyHUD::HandleSettingsPressed()
{
	UStudyMenuSubsystem* MenuSubsystem{
		GetGameInstance()->GetSubsystem<UStudyMenuSubsystem>()};
	SettingsWidget->ShowApiKey(MenuSubsystem->GetApiKey());
	PauseMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	SettingsWidget->SetVisibility(ESlateVisibility::Visible);
}

void AStudyHUD::HandleApiKeySaved(const FString& ApiKey)
{
	UStudyMenuSubsystem* MenuSubsystem{
		GetGameInstance()->GetSubsystem<UStudyMenuSubsystem>()};
	MenuSubsystem->SaveApiKey(ApiKey);
	ShowPauseMenuOnly();
}

void AStudyHUD::HandleMainMenuPressed()
{
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, TitleLevel);
}

void AStudyHUD::HandleQuitPressed()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayerController(),
		EQuitPreference::Quit, false);
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
