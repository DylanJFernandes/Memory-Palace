// Copyright Epic Games, Inc. All Rights Reserved.

#include "Component/StudyWhiteboardComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Game/StudyPadSubsystem.h"
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
	BindToStudyPadSubsystem();
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

void UStudyWhiteboardComponent::BindToStudyPadSubsystem()
{
	UStudyPadSubsystem* StudyPadSubsystem{
		GetWorld()->GetSubsystem<UStudyPadSubsystem>()};
	if (!StudyPadSubsystem)
	{
		return;
	}
	CachedStudyPadSubsystem = StudyPadSubsystem;
	StudyPadSubsystem->OnWorkImageUpdated.AddUObject(
		this, &UStudyWhiteboardComponent::HandleWorkImageUpdated);
	HandleWorkImageUpdated(
		StudyPadSubsystem->GetLatestTexture(),
		StudyPadSubsystem->GetLatestAspectRatio());
}

void UStudyWhiteboardComponent::HandleWorkImageUpdated(
	UTexture2D* WorkTexture, float AspectRatio)
{
	if (!WorkTexture || !WhiteboardMaterial)
	{
		return;
	}
	WhiteboardMaterial->SetTextureParameterValue(
		TextureParameterName, WorkTexture);
}

void UStudyWhiteboardComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	if (CachedStudyPadSubsystem.IsValid())
	{
		CachedStudyPadSubsystem->OnWorkImageUpdated.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}
