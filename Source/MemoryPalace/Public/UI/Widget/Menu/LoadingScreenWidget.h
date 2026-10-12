// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoadingScreenWidget.generated.h"

class UProgressBar;
class UTextBlock;

/* Picture with a progress bar shown while a world is built */
UCLASS()
class MEMORYPALACE_API ULoadingScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/* Shows how far building has come and what is happening */
	void ShowProgress(float Progress,
					  const FString& StatusMessage);

private:
	/* Bar across the bottom of the screen */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> LoadingProgressBar;

	/* What is happening right now */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatusText;
};
