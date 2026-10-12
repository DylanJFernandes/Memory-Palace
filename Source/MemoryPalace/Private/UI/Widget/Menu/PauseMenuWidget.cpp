// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Menu/PauseMenuWidget.h"
#include "Components/Button.h"

void UPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResumeButton->OnClicked.AddDynamic(
		this, &UPauseMenuWidget::HandleResumeClicked);
	SettingsButton->OnClicked.AddDynamic(
		this, &UPauseMenuWidget::HandleSettingsClicked);
	MainMenuButton->OnClicked.AddDynamic(
		this, &UPauseMenuWidget::HandleMainMenuClicked);
	QuitButton->OnClicked.AddDynamic(
		this, &UPauseMenuWidget::HandleQuitClicked);
}

void UPauseMenuWidget::HandleResumeClicked()
{
	OnResumePressed.ExecuteIfBound();
}

void UPauseMenuWidget::HandleSettingsClicked()
{
	OnSettingsPressed.ExecuteIfBound();
}

void UPauseMenuWidget::HandleMainMenuClicked()
{
	OnMainMenuPressed.ExecuteIfBound();
}

void UPauseMenuWidget::HandleQuitClicked()
{
	OnQuitPressed.ExecuteIfBound();
}
