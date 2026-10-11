// Copyright Epic Games, Inc. All Rights Reserved.

#include "Component/StudyWhiteboardComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Game/StudyPadImageSubsystem.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	constexpr int32 WhiteboardMaterialSlotIndex{0};
}

UStudyWhiteboardComponent::UStudyWhiteboardComponent()
	: TextureParameterName{TEXT("WhiteboardTexture")}
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStudyWhiteboardComponent::BeginPlay()
{
	Super::BeginPlay();
	CreateWhiteboardMaterial();
	BindToImageSubsystem();
}

void UStudyWhiteboardComponent::CreateWhiteboardMaterial()
{
	UStaticMeshComponent* WhiteboardMesh{
		GetOwner()->FindComponentByClass<UStaticMeshComponent>()};
	if (!WhiteboardMesh)
	{
		return;
	}
	WhiteboardMaterial =
		WhiteboardMesh->CreateAndSetMaterialInstanceDynamic(
			WhiteboardMaterialSlotIndex);
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
	if (!ReceivedTexture || !WhiteboardMaterial)
	{
		return;
	}
	WhiteboardMaterial->SetTextureParameterValue(
		TextureParameterName, ReceivedTexture);
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
