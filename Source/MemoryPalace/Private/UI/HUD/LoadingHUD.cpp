// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/HUD/LoadingHUD.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Game/StudyMenuSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UI/Widget/Menu/LoadingScreenWidget.h"

namespace
{
	const FString ReadyStatus{TEXT("ready")};
	const FString FailedStatus{TEXT("failed")};
	constexpr float ProgressCreepPerCheck{0.02f};
	constexpr float MaximumCreepProgress{0.95f};
	constexpr float ReadyDelaySeconds{1.0f};
	constexpr float FailedDelaySeconds{5.0f};
}

ALoadingHUD::ALoadingHUD()
	: StatusIntervalSeconds{0.5f}
	, DisplayedProgress{0.0f}
	, bIsOpeningGameLevel{false}
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALoadingHUD::BeginPlay()
{
	Super::BeginPlay();
	CreateLoadingScreenWidget();
	StartBuilding();
}

void ALoadingHUD::CreateLoadingScreenWidget()
{
	LoadingScreenWidget = CreateWidget<ULoadingScreenWidget>(
		GetOwningPlayerController(), LoadingScreenWidgetClass);
	LoadingScreenWidget->AddToViewport();
	LoadingScreenWidget->ShowProgress(
		0.0f, TEXT("Building your palace..."));
}

void ALoadingHUD::StartBuilding()
{
	UStudyMenuSubsystem* MenuSubsystem{GetMenuSubsystem()};
	MenuSubsystem->OnMaterialsStatusReceived.BindUObject(
		this, &ALoadingHUD::HandleBuildStatusReceived);
	MenuSubsystem->RequestGeneration();
	GetWorldTimerManager().SetTimer(StatusTimerHandle, this,
		&ALoadingHUD::RequestBuildStatus, StatusIntervalSeconds,
		true);
}

void ALoadingHUD::RequestBuildStatus()
{
	GetMenuSubsystem()->RequestMaterialsStatus();
}

void ALoadingHUD::HandleBuildStatusReceived(float Progress,
	const FString& Status, const FString& Message)
{
	if (bIsOpeningGameLevel)
	{
		return;
	}
	const bool bIsReady{Status == ReadyStatus};
	DisplayedProgress =
		bIsReady ? 1.0f : ComputeDisplayedProgress(Progress);
	LoadingScreenWidget->ShowProgress(DisplayedProgress, Message);
	if (bIsReady)
	{
		OpenGameLevelSoon(ReadyDelaySeconds);
	}
	else if (Status == FailedStatus)
	{
		LoadingScreenWidget->ShowProgress(DisplayedProgress,
			Message + TEXT(" Opening with the previous problems."));
		OpenGameLevelSoon(FailedDelaySeconds);
	}
}

float ALoadingHUD::ComputeDisplayedProgress(
	float ServerProgress) const
{
	const float CreptProgress{FMath::Min(
		DisplayedProgress + ProgressCreepPerCheck,
		MaximumCreepProgress)};
	return FMath::Max(ServerProgress, CreptProgress);
}

void ALoadingHUD::OpenGameLevelSoon(float DelaySeconds)
{
	bIsOpeningGameLevel = true;
	GetWorldTimerManager().ClearTimer(StatusTimerHandle);
	GetWorldTimerManager().SetTimer(OpenLevelTimerHandle, this,
		&ALoadingHUD::OpenGameLevel, DelaySeconds, false);
}

void ALoadingHUD::OpenGameLevel()
{
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, GameLevel);
}

UStudyMenuSubsystem* ALoadingHUD::GetMenuSubsystem() const
{
	return GetGameInstance()->GetSubsystem<UStudyMenuSubsystem>();
}
