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
	, InitialMeshScale{FVector::OneVector}
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
	CachedWhiteboardMesh = WhiteboardMesh;
	InitialMeshScale = WhiteboardMesh->GetRelativeScale3D();
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
	ApplyAspectRatio(AspectRatio);
}

void UStudyWhiteboardComponent::ApplyAspectRatio(
	float AspectRatio)
{
	if (!CachedWhiteboardMesh.IsValid())
	{
		return;
	}
	FVector NewScale{InitialMeshScale};
	NewScale.Y = InitialMeshScale.Z * AspectRatio;
	CachedWhiteboardMesh->SetRelativeScale3D(NewScale);
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
