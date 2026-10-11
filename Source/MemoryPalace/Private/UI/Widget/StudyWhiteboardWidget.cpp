// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/StudyWhiteboardWidget.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void UStudyWhiteboardWidget::SetWhiteboardTexture(
	UTexture2D* WhiteboardTexture)
{
	WhiteboardImage->SetBrushFromTexture(WhiteboardTexture);
}
