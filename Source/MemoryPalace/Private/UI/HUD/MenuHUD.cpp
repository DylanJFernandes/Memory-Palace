// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/HUD/MenuHUD.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Game/StudyMenuSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UI/Widget/Menu/CreateWorldWidget.h"
#include "UI/Widget/Menu/SettingsWidget.h"
#include "UI/Widget/Menu/TitleScreenWidget.h"
#include "UI/Widget/Menu/WorldSelectWidget.h"

AMenuHUD::AMenuHUD()
	: UploadStatusIntervalSeconds{2.0f}
{
	PrimaryActorTick.bCanEverTick = false;
}

void AMenuHUD::BeginPlay()
{
	Super::BeginPlay();
	CreateScreens();
	BindScreenEvents();
	BindMenuSubsystemEvents();
	ShowTitleScreen();
}

void AMenuHUD::CreateScreens()
{
	TitleScreenWidget = CreateScreen(TitleScreenWidgetClass);
	SettingsWidget = CreateScreen(SettingsWidgetClass);
	WorldSelectWidget = CreateScreen(WorldSelectWidgetClass);
	CreateWorldWidget = CreateScreen(CreateWorldWidgetClass);
}

template <typename WidgetType>
WidgetType* AMenuHUD::CreateScreen(
	TSubclassOf<WidgetType> WidgetClass)
{
	WidgetType* Screen{CreateWidget<WidgetType>(
		GetOwningPlayerController(), WidgetClass)};
	Screen->AddToViewport();
	Screen->SetVisibility(ESlateVisibility::Collapsed);
	return Screen;
}

void AMenuHUD::BindScreenEvents()
{
	TitleScreenWidget->OnSinglePlayerPressed.BindUObject(
		this, &AMenuHUD::ShowWorldSelect);
	TitleScreenWidget->OnSettingsPressed.BindUObject(
		this, &AMenuHUD::ShowSettings);
	SettingsWidget->OnApiKeySaved.BindUObject(
		this, &AMenuHUD::HandleApiKeySaved);
	SettingsWidget->OnBackPressed.BindUObject(
		this, &AMenuHUD::ShowTitleScreen);
	WorldSelectWidget->OnWorldChosen.BindUObject(
		this, &AMenuHUD::PlayWorld);
	WorldSelectWidget->OnCreateWorldPressed.BindUObject(
		this, &AMenuHUD::ShowCreateWorld);
	WorldSelectWidget->OnCancelPressed.BindUObject(
		this, &AMenuHUD::ShowTitleScreen);
	CreateWorldWidget->OnCreatePressed.BindUObject(
		this, &AMenuHUD::CreateWorld);
	CreateWorldWidget->OnCancelPressed.BindUObject(
		this, &AMenuHUD::CancelCreateWorld);
}

void AMenuHUD::BindMenuSubsystemEvents()
{
	UStudyMenuSubsystem* MenuSubsystem{GetMenuSubsystem()};
	MenuSubsystem->OnQrCodeReceived.BindUObject(
		this, &AMenuHUD::HandleQrCodeReceived);
	MenuSubsystem->OnMaterialsStatusReceived.BindUObject(
		this, &AMenuHUD::HandleUploadStatusReceived);
}

void AMenuHUD::ShowScreen(UUserWidget* Screen)
{
	const TArray<UUserWidget*> Screens{TitleScreenWidget,
		SettingsWidget, WorldSelectWidget, CreateWorldWidget};
	for (UUserWidget* Candidate : Screens)
	{
		Candidate->SetVisibility(Candidate == Screen
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
	}
}

void AMenuHUD::ShowTitleScreen()
{
	ShowScreen(TitleScreenWidget);
}

void AMenuHUD::ShowWorldSelect()
{
	WorldSelectWidget->ShowWorlds(
		GetMenuSubsystem()->GetWorldNames());
	ShowScreen(WorldSelectWidget);
}

void AMenuHUD::ShowSettings()
{
	SettingsWidget->ShowApiKey(GetMenuSubsystem()->GetApiKey());
	ShowScreen(SettingsWidget);
}

void AMenuHUD::HandleApiKeySaved(const FString& ApiKey)
{
	GetMenuSubsystem()->SaveApiKey(ApiKey);
	ShowTitleScreen();
}

void AMenuHUD::ShowCreateWorld()
{
	ShowScreen(CreateWorldWidget);
	GetMenuSubsystem()->RequestQrCode();
	GetWorldTimerManager().SetTimer(UploadStatusTimerHandle, this,
		&AMenuHUD::RequestUploadStatus,
		UploadStatusIntervalSeconds, true);
}

void AMenuHUD::CancelCreateWorld()
{
	GetWorldTimerManager().ClearTimer(UploadStatusTimerHandle);
	ShowWorldSelect();
}

void AMenuHUD::RequestUploadStatus()
{
	GetMenuSubsystem()->RequestMaterialsStatus();
}

void AMenuHUD::HandleQrCodeReceived(UTexture2D* QrCodeTexture)
{
	CreateWorldWidget->ShowQrCode(QrCodeTexture);
}

void AMenuHUD::HandleUploadStatusReceived(float Progress,
	const FString& Status, const FString& Message)
{
	CreateWorldWidget->ShowUploadStatus(Message);
}

void AMenuHUD::CreateWorld(const FString& WorldName)
{
	GetWorldTimerManager().ClearTimer(UploadStatusTimerHandle);
	GetMenuSubsystem()->AddWorld(WorldName);
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, LoadingLevel);
}

void AMenuHUD::PlayWorld(const FString& WorldName)
{
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, GameLevel);
}

UStudyMenuSubsystem* AMenuHUD::GetMenuSubsystem() const
{
	return GetGameInstance()->GetSubsystem<UStudyMenuSubsystem>();
}
