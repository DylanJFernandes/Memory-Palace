// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Menu/TitleScreenWidget.h"
#include "Components/Button.h"

void UTitleScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	MultiplayerButton->SetIsEnabled(false);
	SinglePlayerButton->OnClicked.AddDynamic(
		this, &UTitleScreenWidget::HandleSinglePlayerClicked);
	SettingsButton->OnClicked.AddDynamic(
		this, &UTitleScreenWidget::HandleSettingsClicked);
}

void UTitleScreenWidget::HandleSinglePlayerClicked()
{
	OnSinglePlayerPressed.ExecuteIfBound();
}

void UTitleScreenWidget::HandleSettingsClicked()
{
	OnSettingsPressed.ExecuteIfBound();
}
