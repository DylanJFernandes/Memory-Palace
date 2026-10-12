// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widget/StudyWhiteboardBrowserWidget.h"
#include "Data/StudyServerAddress.h"
#include "WebBrowser.h"

void UStudyWhiteboardBrowserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	StudyWebBrowser->LoadURL(StudyServerBaseUrl);
}
