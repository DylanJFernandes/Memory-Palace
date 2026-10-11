// Copyright Epic Games, Inc. All Rights Reserved.

#include "Component/StudyWhiteboardComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Texture2D.h"
#include "Game/StudyPadImageSubsystem.h"
#include "UI/Widget/StudyWhiteboardWidget.h"

UStudyWhiteboardComponent::UStudyWhiteboardComponent()
	: WidgetDrawSize{1024.0, 768.0}
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStudyWhiteboardComponent::BeginPlay()
{
	Super::BeginPlay();
	CreateWidgetComponent();
	BindToImageSubsystem();
}

void UStudyWhiteboardComponent::CreateWidgetComponent()
{
	WidgetComponent = NewObject<UWidgetComponent>(GetOwner());
	WidgetComponent->SetupAttachment(this);
	WidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	WidgetComponent->SetWidgetClass(WidgetClass);
	WidgetComponent->SetDrawSize(WidgetDrawSize);
	WidgetComponent->RegisterComponent();
	WidgetComponent->InitWidget();
}

void UStudyWhiteboardComponent::BindToImageSubsystem()
{
	UStudyPadImageSubsystem* ImageSubsystem{
		GetWorld()->GetSubsystem<UStudyPadImageSubsystem>()};
	if (!ImageSubsystem)
	{
		return;
	}
	CachedImageSubsystem = ImageSubsystem;
	ImageSubsystem->OnImageReceived.BindUObject(
		this, &UStudyWhiteboardComponent::HandleImageReceived);
	HandleImageReceived(ImageSubsystem->GetLatestTexture());
}

void UStudyWhiteboardComponent::HandleImageReceived(
	UTexture2D* ReceivedTexture)
{
	if (!ReceivedTexture)
	{
		return;
	}
	UStudyWhiteboardWidget* WhiteboardWidget{
		Cast<UStudyWhiteboardWidget>(
			WidgetComponent->GetUserWidgetObject())};
	if (!WhiteboardWidget)
	{
		return;
	}
	WhiteboardWidget->SetWhiteboardTexture(ReceivedTexture);
}

void UStudyWhiteboardComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	if (CachedImageSubsystem.IsValid())
	{
		CachedImageSubsystem->OnImageReceived.Unbind();
	}
	Super::EndPlay(EndPlayReason);
}
