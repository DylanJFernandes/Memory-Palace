// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StudyWhiteboardWidget.generated.h"

class UImage;
class UTexture2D;

/* Displays the study pad whiteboard image */
UCLASS()
class MEMORYPALACE_API UStudyWhiteboardWidget
	: public UUserWidget
{
	GENERATED_BODY()

public:
	/* Shows the given texture on the whiteboard image */
	void SetWhiteboardTexture(UTexture2D* WhiteboardTexture);

private:
	/* Image that shows the whiteboard texture */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> WhiteboardImage;
};
