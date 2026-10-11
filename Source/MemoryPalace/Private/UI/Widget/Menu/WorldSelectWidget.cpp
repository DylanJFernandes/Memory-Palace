// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Menu/WorldSelectWidget.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"

void UWorldSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();
	CreateWorldButton->OnClicked.AddDynamic(
		this, &UWorldSelectWidget::HandleCreateWorldClicked);
	CancelButton->OnClicked.AddDynamic(
		this, &UWorldSelectWidget::HandleCancelClicked);
}

void UWorldSelectWidget::ShowWorlds(
	const TArray<FString>& WorldNames)
{
	WorldListScrollBox->ClearChildren();
	if (!WorldEntryWidgetClass)
	{
		return;
	}
	for (const FString& WorldName : WorldNames)
	{
		UWorldEntryWidget* WorldEntry{
			CreateWidget<UWorldEntryWidget>(
				this, WorldEntryWidgetClass)};
		WorldEntry->ShowWorld(WorldName);
		WorldEntry->OnWorldChosen.BindUObject(
			this, &UWorldSelectWidget::HandleWorldChosen);
		WorldListScrollBox->AddChild(WorldEntry);
	}
}

void UWorldSelectWidget::HandleWorldChosen(
	const FString& WorldName)
{
	OnWorldChosen.ExecuteIfBound(WorldName);
}

void UWorldSelectWidget::HandleCreateWorldClicked()
{
	OnCreateWorldPressed.ExecuteIfBound();
}

void UWorldSelectWidget::HandleCancelClicked()
{
	OnCancelPressed.ExecuteIfBound();
}
