// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StudyWhiteboardBrowserWidget.generated.h"

class UWebBrowser;

/* Shows the study pad web page, so the whiteboard can be used
   with every tool the iPad has */
UCLASS()
class MEMORYPALACE_API UStudyWhiteboardBrowserWidget
	: public UUserWidget
{
	GENERATED_BODY()

protected:
	/* Opens the study pad page in the browser */
	virtual void NativeConstruct() override;

private:
	/* Browser that displays the study pad page */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWebBrowser> StudyWebBrowser;
};
