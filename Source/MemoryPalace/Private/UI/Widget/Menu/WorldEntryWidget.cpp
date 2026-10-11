// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Menu/WorldEntryWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UWorldEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	PlayButton->OnClicked.AddDynamic(
		this, &UWorldEntryWidget::HandlePlayClicked);
}

void UWorldEntryWidget::ShowWorld(const FString& InWorldName)
{
	WorldName = InWorldName;
	WorldNameText->SetText(FText::FromString(WorldName));
}

void UWorldEntryWidget::HandlePlayClicked()
{
	OnWorldChosen.ExecuteIfBound(WorldName);
}
