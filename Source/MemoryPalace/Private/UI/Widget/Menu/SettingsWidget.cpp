// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/Menu/SettingsWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"

void USettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SaveButton->OnClicked.AddDynamic(
		this, &USettingsWidget::HandleSaveClicked);
	BackButton->OnClicked.AddDynamic(
		this, &USettingsWidget::HandleBackClicked);
}

void USettingsWidget::ShowApiKey(const FString& ApiKey)
{
	ApiKeyTextBox->SetText(FText::FromString(ApiKey));
}

void USettingsWidget::HandleSaveClicked()
{
	OnApiKeySaved.ExecuteIfBound(
		ApiKeyTextBox->GetText().ToString());
}

void USettingsWidget::HandleBackClicked()
{
	OnBackPressed.ExecuteIfBound();
}
